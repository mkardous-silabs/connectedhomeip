/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
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

/**
 *    @file
 *          Provides an implementation of the BLEManager singleton object
 *          for the Silicon Labs EFR32 platforms.
 */
#pragma once
#include <platform/internal/CHIPDeviceLayerInternal.h>
#include <platform/silabs/BleManagerAbstraction.h>

namespace chip {
namespace DeviceLayer {
namespace Internal {

class BLEManagerImpl final : public BleManagerAbstraction
{

public:
    ~BLEManagerImpl() = default;

private:
    CHIP_ERROR SilabsSendIndication(uint8_t connectionId, uint16_t characteristicId, size_t length, uint8_t * data);
    CHIP_ERROR SilabsSetAdvertiserData(size_t advDataLength, uint8_t * advData, size_t responseDataLength, uint8_t * responseData);
    CHIP_ERROR SilabsConfigureRandomAddress();
    CHIP_ERROR SilabsStartAdvertising(uint32_t minInterval, uint32_t maxInterval);
    CHIP_ERROR SilabsStopAdvertising();
    CHIP_ERROR SilabsConnectionClose(BLE_CONNECTION_OBJECT conId);
    CHIP_ERROR MapBLEError(int bleErr);

    // The advertising set handle allocated from Bluetooth stack.
    uint8_t advertising_set_handle = 0xff;

    // Allow the BLEManager interface class to delegate method calls to
    // the implementation methods provided by this class.
    friend BLEManager;

    // ===== Members for internal use by the following friends.

    friend BLEManager & BLEMgr(void);
    friend BLEManagerImpl & BLEMgrImpl(void);

    static BLEManagerImpl sInstance;
};

/**
 * Returns a reference to the public interface of the BLEManager singleton object.
 *
 * Internal components should use this to access features of the BLEManager object
 * that are common to all platforms.
 */
inline BLEManager & BLEMgr(void)
{
    return BLEManagerImpl::sInstance;
}

/**
 * Returns the platform-specific implementation of the BLEManager singleton object.
 *
 * Internal components can use this to gain access to features of the BLEManager
 * that are specific to the EFR32 platforms.
 */
inline BLEManagerImpl & BLEMgrImpl(void)
{
    return BLEManagerImpl::sInstance;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip
