/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
 *    Copyright (c) 2019 Nest Labs, Inc.
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
 *          Provides an Abstract Template class of the BLEManager singleton object
 *          for Silicon Labs platforms.
 */

#pragma once
#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE

#include "FreeRTOS.h"
#include "sl_bt_api.h"
#include "timers.h"

namespace chip {
namespace DeviceLayer {
namespace Internal {

using namespace chip::Ble;

/**
 * Abstract template implementation of the BLEManager singleton object for Silicon Labs platforms.
 */
class BleManagerAbstraction : public BLEManager, private BleLayer, private BlePlatformDelegate, private BleApplicationDelegate
{
public:
    // ===== Members that implement Silabs interface
    void HandleBootEvent(void);
    void HandleConnectEvent(sl_bt_msg_t * evt);
    void HandleConnectionCloseEvent(sl_bt_msg_t * evt);
    void HandleWriteEvent(sl_bt_msg_t * evt);
    void UpdateMtu(sl_bt_msg_t * evt);
    void HandleTxConfirmationEvent(BLE_CONNECTION_OBJECT conId);
    void HandleTXCharCCCDWrite(sl_bt_msg_t * evt);
    CHIP_ERROR StartAdvertising(void);

#if CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING
    void HandleC3ReadRequest(sl_bt_msg_t * evt);
#endif // CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING

protected:
    // ===== Platform Abstraction Implementation Functions
    virtual CHIP_ERROR SilabsInitBLEManager()                                                                               = 0;
    virtual CHIP_ERROR SilabsSendIndication(uint8_t connectionId, uint16_t characteristicId, size_t length, uint8_t * data) = 0;
    virtual CHIP_ERROR MapBLEError(int bleErr)                                                                              = 0;
    virtual CHIP_ERROR SilabsSetAdvertiserData(size_t advDataLength, uint8_t * advData, size_t responseDataLength,
                                               uint8_t * responseData)                                                      = 0;
    virtual CHIP_ERROR SilabsConfigureRandomAddress()                                                                       = 0;
    virtual CHIP_ERROR SilabsStartAdvertising(uint32_t minInterval, uint32_t maxInterval)                                   = 0;
    virtual CHIP_ERROR SilabsStopAdvertising()                                                                              = 0;
    virtual CHIP_ERROR SilabsConnectionClose(BLE_CONNECTION_OBJECT conId)                                                   = 0;

    static constexpr uint8_t kMaxConnections      = BLE_LAYER_NUM_BLE_ENDPOINTS;
    static constexpr uint8_t kMaxDeviceNameLength = 16;
    static constexpr uint8_t kUnusedIndex         = 0xFF;

private:
    // Allow the BLEManager interface class to delegate method calls to
    // the implementation methods provided by this class.
    friend BLEManager;

    // ===== Members that implement the BLEManager internal interface.

    CHIP_ERROR _Init(void);
    void _Shutdown() {}
    bool _IsAdvertisingEnabled(void);
    CHIP_ERROR _SetAdvertisingEnabled(bool val);
    bool _IsAdvertising(void);
    CHIP_ERROR _SetAdvertisingMode(BLEAdvertisingMode mode);
    CHIP_ERROR _GetDeviceName(char * buf, size_t bufSize);
    CHIP_ERROR _SetDeviceName(const char * deviceName);
    uint16_t _NumConnections(void);
    void _OnPlatformEvent(const ChipDeviceEvent * event);
    BleLayer * _GetBleLayer(void);

    // ===== Members that implement virtual methods on BlePlatformDelegate.

    bool SubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const Ble::ChipBleUUID * svcId,
                                 const Ble::ChipBleUUID * charId) override;
    bool UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const Ble::ChipBleUUID * svcId,
                                   const Ble::ChipBleUUID * charId) override;
    bool CloseConnection(BLE_CONNECTION_OBJECT conId) override;
    uint16_t GetMTU(BLE_CONNECTION_OBJECT conId) const override;
    bool SendIndication(BLE_CONNECTION_OBJECT conId, const Ble::ChipBleUUID * svcId, const Ble::ChipBleUUID * charId,
                        System::PacketBufferHandle pBuf) override;
    bool SendWriteRequest(BLE_CONNECTION_OBJECT conId, const Ble::ChipBleUUID * svcId, const Ble::ChipBleUUID * charId,
                          System::PacketBufferHandle pBuf) override;
    bool SendReadRequest(BLE_CONNECTION_OBJECT conId, const Ble::ChipBleUUID * svcId, const Ble::ChipBleUUID * charId,
                         System::PacketBufferHandle pBuf) override;
    bool SendReadResponse(BLE_CONNECTION_OBJECT conId, BLE_READ_REQUEST_CONTEXT requestContext, const Ble::ChipBleUUID * svcId,
                          const Ble::ChipBleUUID * charId) override;

    // ===== Members that implement virtual methods on BleApplicationDelegate.

    void NotifyChipConnectionClosed(BLE_CONNECTION_OBJECT conId) override;

    // ===== Private members reserved for use by this class only.

    enum class Flags : uint16_t
    {
        kAdvertisingEnabled        = 0x0001,
        kFastAdvertisingEnabled    = 0x0002,
        kAdvertising               = 0x0004,
        kRestartAdvertising        = 0x0008,
        kSilabsBLEStackInitialized = 0x0010,
        kDeviceNameSet             = 0x0020,
    };

    struct CHIPoBLEConState
    {
        uint16_t mtu : 10;
        uint16_t allocated : 1;
        uint16_t subscribed : 1;
        uint16_t unused : 4;
        uint8_t connectionHandle;
        uint8_t bondingHandle;
    };

    CHIPoBLEConState mBleConnections[kMaxConnections];
    CHIPoBLEServiceMode mServiceMode;
    BitFlags<Flags> mFlags;
    char mDeviceName[kMaxDeviceNameLength + 1];

#if CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING
    PacketBufferHandle c3AdditionalDataBufferHandle;
#endif

    void DriveBLEState(void);
    CHIP_ERROR ConfigureAdvertisingData(void);
    CHIP_ERROR StopAdvertising(void);
    void HandleRXCharWrite(sl_bt_msg_t * evt);
    bool RemoveConnection(uint8_t connectionHandle);
    void AddConnection(uint8_t connectionHandle, uint8_t bondingHandle);
    void StartBleAdvTimeoutTimer(uint32_t aTimeoutInMs);
    void CancelBleAdvTimeoutTimer(void);
    CHIPoBLEConState * GetConnectionState(uint8_t conId, bool allocate = false);
    static void DriveBLEState(intptr_t arg);
    static void BleAdvTimeoutHandler(TimerHandle_t xTimer);
    static void HandleSoftTimerEvent(System::Layer * apSystemLayer, void * apAppState);

#if CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING
    CHIP_ERROR EncodeAdditionalDataTlv();
#endif
};

inline BleLayer * BleManagerAbstraction::_GetBleLayer()
{
    return this;
}

inline bool BleManagerAbstraction::_IsAdvertisingEnabled(void)
{
    return mFlags.Has(Flags::kAdvertisingEnabled);
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

#endif // CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
