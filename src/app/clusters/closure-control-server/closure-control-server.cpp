/*
 *    Copyright (c) 2025 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "closure-control-server.h"

#include <app/AttributeAccessInterface.h>
#include <app/AttributeAccessInterfaceRegistry.h>
#include <app/CommandHandlerInterfaceRegistry.h>
#include <app/ConcreteAttributePath.h>
#include <app/InteractionModelEngine.h>
#include <app/util/attribute-storage.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::DataModel;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ClosureControl;
using namespace chip::app::Clusters::ClosureControl::Attributes;
using namespace chip::app::Clusters::ClosureControl::Commands;
using chip::Protocols::InteractionModel::Status;

namespace chip {
namespace app {
namespace Clusters {
namespace ClosureControl {

CHIP_ERROR Instance::Init()
{
    ReturnErrorOnFailure(CommandHandlerInterfaceRegistry::Instance().RegisterCommandHandler(this));
    VerifyOrReturnError(AttributeAccessInterfaceRegistry::Instance().Register(this), CHIP_ERROR_INCORRECT_STATE);

    return CHIP_NO_ERROR;
}

void Instance::Shutdown()
{
    CommandHandlerInterfaceRegistry::Instance().UnregisterCommandHandler(this);
    AttributeAccessInterfaceRegistry::Instance().Unregister(this);
}

bool Instance::HasFeature(Feature aFeatures) const
{
    return mFeatures.Has(aFeatures);
}

bool Instance::SupportsOptAttr(OptionalAttribute aOptionalAttrs) const
{
    return mOptionalAttrs.Has(aOptionalAttrs);
}

bool Instance::IsSupportedMainState(MainStateEnum aMainState)
{
    bool isSupported = false;

    switch (aMainState)
    {
    case MainStateEnum::kCalibrating:
        isSupported = HasFeature(Feature::kCalibration);
        break;
    case MainStateEnum::kProtected:
        isSupported = HasFeature(Feature::kProtection);
        break;
    case MainStateEnum::kDisengaged:
        isSupported = HasFeature(Feature::kManuallyOperable);
        break;
    default:
        // Remaining MainState have Mandatory conformance, so will be supported.
        isSupported = true;
        break;
    }

    return isSupported;
}

CHIP_ERROR Instance::SetMainState(MainStateEnum aMainState)
{
    // Where is the validation for the main state transition? The attribute defines the state machines that needs to be implemented

    VerifyOrReturnError(IsSupportedMainState(aMainState), CHIP_ERROR_INVALID_ARGUMENT);

    // If the Main State has changed, trigger the attribute change callback
    VerifyOrReturnError(mMainState != aMainState, CHIP_NO_ERROR);

    mMainState = aMainState;
    MatterReportingAttributeChangeCallback(mDelegate.GetEndpointId(), ClosureControl::Id, Attributes::MainState::Id);
    UpdateCountdownTimeFromClusterLogic();

    return CHIP_NO_ERROR;
}

CHIP_ERROR Instance::SetOverallState(const GenericOverallState & aOverallState)
{
    // If the overall state has changed, trigger the attribute change callback
    VerifyOrReturnError(mOverallState != aOverallState, CHIP_NO_ERROR);

    mOverallState = aOverallState;
    MatterReportingAttributeChangeCallback(mDelegate.GetEndpointId(), ClosureControl::Id, Attributes::OverallState::Id);

    return CHIP_NO_ERROR;
}

CHIP_ERROR Instance::SetOverallTarget(const GenericOverallTarget & aOverallTarget)
{

    // If the overall target has changed, trigger the attribute change callback
    VerifyOrReturnError(mOverallTarget != aOverallTarget, CHIP_NO_ERROR);

    mOverallTarget = aOverallTarget;
    MatterReportingAttributeChangeCallback(mDelegate.GetEndpointId(), ClosureControl::Id, Attributes::OverallTarget::Id);

    return CHIP_NO_ERROR;
}

MainStateEnum Instance::GetMainState() const
{
    return mMainState;
}

const GenericOverallState & Instance::GetOverallState() const
{
    return mOverallState;
}

const GenericOverallTarget & Instance::GetOverallTarget() const
{
    return mOverallTarget;
}

void Instance::UpdateCountdownTime(bool fromDelegate)
{

    app::DataModel::Nullable<uint32_t> newCountdownTime /* isnt the best name since it is not garanteed that the value is new */ =
        mDelegate.GetCountdownTime();
    auto now = System::SystemClock().GetMonotonicTimestamp();

    bool markDirty = false;

    if (fromDelegate)
    {
        // Updates from delegate are reduce-reported to every 1s max (choice of this implementation), in addition
        // to default change-from-null, change-from-zero and increment policy.
        System::Clock::Milliseconds64 reportInterval = System::Clock::Milliseconds64(1000);
        auto predicate                               = mCountdownTime.GetPredicateForSufficientTimeSinceLastDirty(reportInterval);
        markDirty = (mCountdownTime.SetValue(newCountdownTime, now, predicate) == AttributeDirtyState::kMustReport);
    }
    else
    {
        auto predicate = [](const decltype(mCountdownTime)::SufficientChangePredicateCandidate &) -> bool { return true; };
        markDirty      = (mCountdownTime.SetValue(newCountdownTime, now, predicate) == AttributeDirtyState::kMustReport);
    }

    if (markDirty)
    {
        MatterReportingAttributeChangeCallback(mDelegate.GetEndpointId(), ClosureControl::Id, Attributes::CountdownTime::Id);
    }
}

// AttributeAccessInterface
CHIP_ERROR Instance::Read(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder)
{
    VerifyOrReturnError(aPath.mClusterId == ClosureControl::Id, CHIP_ERROR_INVALID_PATH_LIST);

    switch (aPath.mAttributeId)
    {
    case CountdownTime::Id:
        // Optional Attribute
        VerifyOrReturnError(SupportsOptAttr(OptionalAttribute::kCountdownTime), CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute));
        return aEncoder.Encode(mDelegate.GetCountdownTime());

    case MainState::Id:
        return aEncoder.Encode(GetMainState());

    case CurrentErrorList::Id:
        return aEncoder.EncodeList([this](const auto & encoder) -> CHIP_ERROR { return this->EncodeCurrentErrorList(encoder); });

    case OverallState::Id:
        return aEncoder.Encode(GetOverallState());

    case OverallTarget::Id:
        return aEncoder.Encode(GetOverallTarget());

    case FeatureMap::Id:
        // FeatureMap - is held locally
        return aEncoder.Encode(mFeatures);
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR Instance::EncodeCurrentErrorList(const AttributeValueEncoder::ListEncodeHelper & encoder)
{
    ReturnErrorOnFailure(mDelegate.StartCurrentErrorListRead());

    size_t index = 0;

    do
    {
        ClosureErrorEnum error = kUnknownEnumValue;
        CHIP_ERROR err         = mDelegate.GetCurrentErrorListAtIndex(index++, error);

        if (err == CHIP_ERROR_PROVIDER_LIST_EXHAUSTED)
        {
            break; // End of list, return success
        }

        ReturnErrorOnFailure(err); // Return if another error occurred

        // TODO: How do we validate that there are not duplciates?
        ReturnErrorOnFailure(encoder.Encode(error)); // Return if encoding fails
    } while (true);

    return mDelegate.EndCurrentErrorListRead();
}

CHIP_ERROR Instance::Write(const ConcreteDataAttributePath & aPath, AttributeValueDecoder & aDecoder)
{
    VerifyOrReturnError(aPath.mClusterId == ClosureControl::Id, CHIP_ERROR_INVALID_PATH_LIST);

    // Optional Attribute Write Management
    if (aPath.mAttributeId == CountdownTime::Id && SupportsOptAttr(OptionalAttribute::kCountdownTime))
    {
        return CHIP_IM_GLOBAL_STATUS(UnsupportedWrite);
    }

    // Default Attribute Write Management
    switch (aPath.mAttributeId)
    {
    case CountdownTime::Id:
    case MainState::Id:
    case CurrentErrorList::Id:
    case OverallState::Id:
    case OverallTarget::Id:
        return CHIP_IM_GLOBAL_STATUS(UnsupportedWrite);
    default:
        return CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute);
    }
}

// CommandHandlerInterface
void Instance::InvokeCommand(HandlerContext & handlerContext)
{
    using namespace Commands;

    switch (handlerContext.mRequestPath.mCommandId)
    {
    case Stop::Id:
        if (!HasFeature(Feature::kInstantaneous))
        {
            HandleCommand<Stop::DecodableType>(
                handlerContext, [this](HandlerContext & ctx, const auto & commandData) { HandleStop(ctx, commandData); });
        }
        else
        {
            handlerContext.mCommandHandler.AddStatus(handlerContext.mRequestPath, Status::UnsupportedCommand);
        }
        break;
    case MoveTo::Id:
        HandleCommand<MoveTo::DecodableType>(
            handlerContext, [this](HandlerContext & ctx, const auto & commandData) { HandleMoveTo(ctx, commandData); });
        break;
    case Calibrate::Id:
        if (HasFeature(Feature::kCalibration))
        {
            HandleCommand<Calibrate::DecodableType>(
                handlerContext, [this](HandlerContext & ctx, const auto & commandData) { HandleCalibrate(ctx, commandData); });
        }
        else
        {
            handlerContext.mCommandHandler.AddStatus(handlerContext.mRequestPath, Status::UnsupportedCommand);
        }
        break;
    }
}

void Instance::HandleStop(HandlerContext & ctx, const Commands::Stop::DecodableType & commandData)
{
    // I don't think it is a resonable expectation that all the specification rqeuirements be in the app.
    Status status = mDelegate.Stop();
    ctx.mCommandHandler.AddStatus(ctx.mRequestPath, status);
}

void Instance::HandleMoveTo(HandlerContext & ctx, const Commands::MoveTo::DecodableType & commandData)
{

    // I don't think it is a resonable expectation that all the specification rqeuirements be in the app.
    Status status = mDelegate.MoveTo(commandData.position, commandData.latch, commandData.speed);

    ctx.mCommandHandler.AddStatus(ctx.mRequestPath, status);
}

void Instance::HandleCalibrate(HandlerContext & ctx, const Commands::Calibrate::DecodableType & commandData)
{
    // I don't think it is a resonable expectation that all the specification rqeuirements be in the app.
    Status status = mDelegate.Calibrate();
    ctx.mCommandHandler.AddStatus(ctx.mRequestPath, status);
}

} // namespace ClosureControl
} // namespace Clusters
} // namespace app
} // namespace chip

// -----------------------------------------------------------------------------
// Plugin initialization

void MatterClosureControlPluginServerInitCallback() {}
void MatterClosureControlPluginServerShutdownCallback() {}
