/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
 *    All rights reserved.
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

#pragma once

#include <PowerTopologyDelegate.h>
#include <app/util/attribute-storage.h>

namespace chip {
namespace app {
namespace Clusters {
namespace PowerTopology {

class PowerTopologyDelegateImpl : public PowerTopologyDelegate
{
public:
    ~PowerTopologyDelegateImpl() = default;

    CHIP_ERROR GetAvailableEndpointAtIndex(size_t index, EndpointId & endpointId) override;
    CHIP_ERROR GetActiveEndpointAtIndex(size_t index, EndpointId & endpointId) override;

    CHIP_ERROR AddActiveEndpoint(EndpointId endpointId);
    CHIP_ERROR RemoveActiveEndpoint(EndpointId endpointId);

    void InitAvailableEndpointList();

private:
    EndpointId mAvailableEndpointIdList[MAX_ENDPOINT_COUNT];
    EndpointId mActiveEndpointIdList[MAX_ENDPOINT_COUNT];
};

} // namespace PowerTopology
} // namespace Clusters
} // namespace app
} // namespace chip
