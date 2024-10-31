/*
 *    Copyright (c) 2024 Project CHIP Authors
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

#include "SiWxSleepManager.h"
#include "wifi/WifiInterfaceAbstraction.h"
#include <lib/support/logging/CHIPLogging.h>

using namespace chip::app;

// Initialize the static instance
SiWxSleepManager SiWxSleepManager::mInstance;

CHIP_ERROR SiWxSleepManager::Init()
{
    // Initialization logic
    return CHIP_NO_ERROR;
}

void SiWxSleepManager::OnEnterActiveMode()
{
    // Execution logic for entering active mode
}

void SiWxSleepManager::OnEnterIdleMode()
{
    if (sl_matter_wifi_disconnect() != SL_STATUS_OK)
    {
        ChipLogError(AppServer, "SiWxSleepManager failed to disconnect from the Wi-Fi Network!");
    }
}

void SiWxSleepManager::OnSubscriptionEstablished(ReadHandler & aReadHandler)
{
    // Implement logic for when a subscription is established
}

void SiWxSleepManager::OnSubscriptionTerminated(ReadHandler & aReadHandler)
{
    // Implement logic for when a subscription is terminated
}
