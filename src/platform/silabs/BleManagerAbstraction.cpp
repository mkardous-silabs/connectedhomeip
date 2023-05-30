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
 *          Provides an implementation of the BLEManager singleton object
 *          for the Silicon Labs EFR32 platforms.
 */

/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>
#define CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE 1
#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE

#include "sl_component_catalog.h"

#include <platform/silabs/BleManagerAbstraction.h>

#include "gatt_db.h"
#include "sl_bt_api.h"

#include <ble/CHIPBleServiceData.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/DeviceInstanceInfoProvider.h>

#if CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING
#include <setup_payload/AdditionalDataPayloadGenerator.h>
#endif

using namespace ::chip;
using namespace ::chip::Ble;

namespace chip {
namespace DeviceLayer {
namespace Internal {

namespace {

#define CHIP_ADV_DATA_TYPE_FLAGS 0x01
#define CHIP_ADV_DATA_TYPE_UUID 0x03
#define CHIP_ADV_DATA_TYPE_NAME 0x09
#define CHIP_ADV_DATA_TYPE_SERVICE_DATA 0x16

#define CHIP_ADV_DATA_FLAGS 0x06

#define CHIP_ADV_SCAN_RESPONSE_DATA 1
#define CHIP_ADV_SHORT_UUID_LEN 2

#define MAX_RESPONSE_DATA_LEN 31
#define MAX_ADV_DATA_LEN 31

TimerHandle_t sbleAdvTimeoutTimer; // FreeRTOS sw timer.

constexpr uint8_t UUID_CHIPoBLEService[]       = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
                                             0x00, 0x10, 0x00, 0x00, 0xF6, 0xFF, 0x00, 0x00 };
constexpr uint8_t ShortUUID_CHIPoBLEService[]  = { 0xF6, 0xFF };
constexpr ChipBleUUID ChipUUID_CHIPoBLEChar_RX = { { 0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42,
                                                     0x9F, 0x9D, 0x11 } };
constexpr ChipBleUUID ChipUUID_CHIPoBLEChar_TX = { { 0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42,
                                                     0x9F, 0x9D, 0x12 } };

} // namespace

CHIP_ERROR BleManagerAbstraction::_Init()
{
    CHIP_ERROR err;

    // BLE Manager Platform Init
    err = SilabsInitBLEManager();
    SuccessOrExit(err);

    // Initialize the CHIP BleLayer.
    err = BleLayer::Init(this, this, &DeviceLayer::SystemLayer());
    SuccessOrExit(err);

    memset(mBleConnections, 0, sizeof(mBleConnections));
    mServiceMode = ConnectivityManager::kCHIPoBLEServiceMode_Enabled;

    // Create FreeRTOS sw timer for BLE timeouts and interval change.
    sbleAdvTimeoutTimer = xTimerCreate("BleAdvTimer",       // Just a text name, not used by the RTOS kernel
                                       pdMS_TO_TICKS(1),    // == default timer period
                                       false,               // no timer reload (==one-shot)
                                       (void *) this,       // init timer id = ble obj context
                                       BleAdvTimeoutHandler // timer callback handler
    );

    mFlags.ClearAll().Set(Flags::kAdvertisingEnabled, CHIP_DEVICE_CONFIG_CHIPOBLE_ENABLE_ADVERTISING_AUTOSTART);
    mFlags.Set(Flags::kFastAdvertisingEnabled, true);

    PlatformMgr().ScheduleWork(DriveBLEState, reinterpret_cast<intptr_t>(this));
exit:
    return err;
}

uint16_t BleManagerAbstraction::_NumConnections(void)
{
    uint16_t numCons = 0;
    for (uint16_t i = 0; i < kMaxConnections; i++)
    {
        if (mBleConnections[i].allocated)
        {
            numCons++;
        }
    }

    return numCons;
}

CHIP_ERROR BleManagerAbstraction::_SetAdvertisingEnabled(bool val)
{
    VerifyOrReturnError(mServiceMode != ConnectivityManager::kCHIPoBLEServiceMode_NotSupported,
                        CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE);

    if (mFlags.Has(Flags::kAdvertisingEnabled) != val)
    {
        mFlags.Set(Flags::kAdvertisingEnabled, val);
        PlatformMgr().ScheduleWork(DriveBLEState, reinterpret_cast<intptr_t>(this));
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR BleManagerAbstraction::_SetAdvertisingMode(BLEAdvertisingMode mode)
{
    switch (mode)
    {
    case BLEAdvertisingMode::kFastAdvertising:
        mFlags.Set(Flags::kFastAdvertisingEnabled, true);
        break;
    case BLEAdvertisingMode::kSlowAdvertising:
        mFlags.Set(Flags::kFastAdvertisingEnabled, false);
        break;
    default:
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    mFlags.Set(Flags::kRestartAdvertising);
    PlatformMgr().ScheduleWork(DriveBLEState, reinterpret_cast<intptr_t>(this));

    return CHIP_NO_ERROR;
}

CHIP_ERROR BleManagerAbstraction::_GetDeviceName(char * buf, size_t bufSize)
{
    VerifyOrReturnError(strlen(mDeviceName) < bufSize, CHIP_ERROR_BUFFER_TOO_SMALL)

        strcpy(buf, mDeviceName);
    return CHIP_NO_ERROR;
}

CHIP_ERROR BleManagerAbstraction::_SetDeviceName(const char * deviceName)
{
    if (mServiceMode == ConnectivityManager::kCHIPoBLEServiceMode_NotSupported)
    {
        return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
    }
    if (deviceName != NULL && deviceName[0] != 0)
    {
        if (strlen(deviceName) >= kMaxDeviceNameLength)
        {
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
        strcpy(mDeviceName, deviceName);
        mFlags.Set(Flags::kDeviceNameSet);
        mFlags.Set(Flags::kRestartAdvertising);
        ChipLogProgress(DeviceLayer, "Setting device name to : \"%s\"", mDeviceName);
    }
    else
    {
        mDeviceName[0] = 0;
    }

    PlatformMgr().ScheduleWork(DriveBLEState, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

void BleManagerAbstraction::_OnPlatformEvent(const ChipDeviceEvent * event)
{
    switch (event->Type)
    {
    case DeviceEventType::kCHIPoBLESubscribe: {
        ChipDeviceEvent connEstEvent;

        ChipLogProgress(DeviceLayer, "_OnPlatformEvent kCHIPoBLESubscribe");
        HandleSubscribeReceived(event->CHIPoBLESubscribe.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
        connEstEvent.Type = DeviceEventType::kCHIPoBLEConnectionEstablished;
        PlatformMgr().PostEventOrDie(&connEstEvent);
    }
    break;

    case DeviceEventType::kCHIPoBLEUnsubscribe: {
        ChipLogProgress(DeviceLayer, "_OnPlatformEvent kCHIPoBLEUnsubscribe");
        HandleUnsubscribeReceived(event->CHIPoBLEUnsubscribe.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
    }
    break;

    case DeviceEventType::kCHIPoBLEWriteReceived: {
        ChipLogProgress(DeviceLayer, "_OnPlatformEvent kCHIPoBLEWriteReceived");
        HandleWriteReceived(event->CHIPoBLEWriteReceived.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_RX,
                            PacketBufferHandle::Adopt(event->CHIPoBLEWriteReceived.Data));
    }
    break;

    case DeviceEventType::kCHIPoBLEConnectionError: {
        ChipLogProgress(DeviceLayer, "_OnPlatformEvent kCHIPoBLEConnectionError");
        HandleConnectionError(event->CHIPoBLEConnectionError.ConId, event->CHIPoBLEConnectionError.Reason);
    }
    break;

    case DeviceEventType::kCHIPoBLEIndicateConfirm: {
        ChipLogProgress(DeviceLayer, "_OnPlatformEvent kCHIPoBLEIndicateConfirm");
        HandleIndicationConfirmation(event->CHIPoBLEIndicateConfirm.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
    }
    break;

    default:
        ChipLogProgress(DeviceLayer, "_OnPlatformEvent default:  event->Type = %d", event->Type);
        break;
    }
}

bool BleManagerAbstraction::SubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId,
                                                    const ChipBleUUID * charId)
{
    ChipLogProgress(DeviceLayer, "BleManagerAbstraction::SubscribeCharacteristic() not supported");
    return false;
}

bool BleManagerAbstraction::UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId,
                                                      const ChipBleUUID * charId)
{
    ChipLogProgress(DeviceLayer, "BleManagerAbstraction::UnsubscribeCharacteristic() not supported");
    return false;
}

bool BleManagerAbstraction::CloseConnection(BLE_CONNECTION_OBJECT conId)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    ChipLogProgress(DeviceLayer, "Closing BLE GATT connection (con %u)", conId);
    err = SilabsConnectionClose(conId);

    return (err == CHIP_NO_ERROR);
}

uint16_t BleManagerAbstraction::GetMTU(BLE_CONNECTION_OBJECT conId) const
{
    CHIPoBLEConState * conState = const_cast<BleManagerAbstraction *>(this)->GetConnectionState(conId);
    return (conState != NULL) ? conState->mtu : 0;
}

bool BleManagerAbstraction::SendIndication(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                           PacketBufferHandle data)
{
    CHIP_ERROR err              = CHIP_NO_ERROR;
    CHIPoBLEConState * conState = GetConnectionState(conId);
    uint16_t cId                = (UUIDsMatch(&ChipUUID_CHIPoBLEChar_RX, charId) ? gattdb_CHIPoBLEChar_Rx : gattdb_CHIPoBLEChar_Tx);

    VerifyOrExit(((conState != NULL) && (conState->subscribed != 0)), err = CHIP_ERROR_INVALID_ARGUMENT);

    // start timer for indication confirmation
    SystemLayer().StartTimer(System::Clock::Seconds16(1), HandleSoftTimerEvent, conState);

    // Call Silabs Abstraction SendIndication command
    err = SilabsSendIndication(conId, cId, (data->DataLength()), data->Start());

exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "BleManagerAbstraction::SendIndication() failed: %s", ErrorStr(err));
        return false;
    }

    return true;
}

bool BleManagerAbstraction::SendWriteRequest(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                             PacketBufferHandle pBuf)
{
    ChipLogProgress(DeviceLayer, "BleManagerAbstraction::SendWriteRequest() not supported");
    return false;
}

bool BleManagerAbstraction::SendReadRequest(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                            PacketBufferHandle pBuf)
{
    ChipLogProgress(DeviceLayer, "BleManagerAbstraction::SendReadRequest() not supported");
    return false;
}

bool BleManagerAbstraction::SendReadResponse(BLE_CONNECTION_OBJECT conId, BLE_READ_REQUEST_CONTEXT requestContext,
                                             const ChipBleUUID * svcId, const ChipBleUUID * charId)
{
    ChipLogProgress(DeviceLayer, "BleManagerAbstraction::SendReadResponse() not supported");
    return false;
}

void BleManagerAbstraction::NotifyChipConnectionClosed(BLE_CONNECTION_OBJECT conId)
{
    // Nothing to do
}

void BleManagerAbstraction::DriveBLEState(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    // Check if BLE stack is initialized
    VerifyOrExit(mFlags.Has(Flags::kSilabsBLEStackInitialized), /* */);

    // Start advertising if needed...
    if (mServiceMode == ConnectivityManager::kCHIPoBLEServiceMode_Enabled && mFlags.Has(Flags::kAdvertisingEnabled) &&
        NumConnections() < kMaxConnections)
    {
        // Start/re-start advertising if not already started, or if there is a pending change
        // to the advertising configuration.
        if (!mFlags.Has(Flags::kAdvertising) || mFlags.Has(Flags::kRestartAdvertising))
        {
            err = StartAdvertising();
            SuccessOrExit(err);
        }
    }
    // Otherwise, stop advertising if it is enabled.
    else if (mFlags.Has(Flags::kAdvertising))
    {
        err = StopAdvertising();
        SuccessOrExit(err);
    }

exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Disabling CHIPoBLE service due to error: %s", ErrorStr(err));
        mServiceMode = ConnectivityManager::kCHIPoBLEServiceMode_Disabled;
    }
}

CHIP_ERROR BleManagerAbstraction::ConfigureAdvertisingData(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    ChipBLEDeviceIdentificationInfo mDeviceIdInfo;
    uint8_t responseData[MAX_RESPONSE_DATA_LEN];
    uint8_t advData[MAX_ADV_DATA_LEN];
    uint32_t advDataIndex       = 0;
    uint32_t responseDataIndex  = 0;
    uint32_t mDeviceNameLength  = 0;
    uint8_t mDeviceIdInfoLength = 0;

    VerifyOrExit((kMaxDeviceNameLength + 1) < UINT8_MAX, err = CHIP_ERROR_INVALID_ARGUMENT);

    memset(responseData, 0, MAX_RESPONSE_DATA_LEN);
    memset(advData, 0, MAX_ADV_DATA_LEN);

    err = ConfigurationMgr().GetBLEDeviceIdentificationInfo(mDeviceIdInfo);
    SuccessOrExit(err);

    if (!mFlags.Has(Flags::kDeviceNameSet))
    {
        uint16_t discriminator;
        SuccessOrExit(err = GetCommissionableDataProvider()->GetSetupDiscriminator(discriminator));

        snprintf(mDeviceName, sizeof(mDeviceName), "%s%04u", CHIP_DEVICE_CONFIG_BLE_DEVICE_NAME_PREFIX, discriminator);

        mDeviceName[kMaxDeviceNameLength] = 0;
        mDeviceNameLength                 = strlen(mDeviceName);

        VerifyOrExit(mDeviceNameLength < kMaxDeviceNameLength, err = CHIP_ERROR_INVALID_ARGUMENT);
    }

    mDeviceNameLength = strlen(mDeviceName); // Device Name length + length field
    VerifyOrExit(mDeviceNameLength < kMaxDeviceNameLength, err = CHIP_ERROR_INVALID_ARGUMENT);

    mDeviceIdInfoLength = sizeof(mDeviceIdInfo); // Servicedatalen + length+ UUID (Short)
    static_assert(sizeof(mDeviceIdInfo) + CHIP_ADV_SHORT_UUID_LEN + 1 <= UINT8_MAX, "Our length won't fit in a uint8_t");
    static_assert(2 + CHIP_ADV_SHORT_UUID_LEN + sizeof(mDeviceIdInfo) + 1 <= MAX_ADV_DATA_LEN, "Our buffer is not big enough");

    advData[advDataIndex++] = 0x02;                                                                    // length
    advData[advDataIndex++] = CHIP_ADV_DATA_TYPE_FLAGS;                                                // AD type : flags
    advData[advDataIndex++] = CHIP_ADV_DATA_FLAGS;                                                     // AD value
    advData[advDataIndex++] = static_cast<uint8_t>(mDeviceIdInfoLength + CHIP_ADV_SHORT_UUID_LEN + 1); // AD length
    advData[advDataIndex++] = CHIP_ADV_DATA_TYPE_SERVICE_DATA;                                         // AD type : Service Data
    advData[advDataIndex++] = ShortUUID_CHIPoBLEService[0];                                            // AD value
    advData[advDataIndex++] = ShortUUID_CHIPoBLEService[1];
    memcpy(&advData[advDataIndex], (void *) &mDeviceIdInfo, mDeviceIdInfoLength); // AD value
    advDataIndex += mDeviceIdInfoLength;

#if CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING
    ReturnErrorOnFailure(EncodeAdditionalDataTlv());
#endif

    responseData[responseDataIndex++] = CHIP_ADV_SHORT_UUID_LEN + 1;  // AD length
    responseData[responseDataIndex++] = CHIP_ADV_DATA_TYPE_UUID;      // AD type : uuid
    responseData[responseDataIndex++] = ShortUUID_CHIPoBLEService[0]; // AD value
    responseData[responseDataIndex++] = ShortUUID_CHIPoBLEService[1];

    responseData[responseDataIndex++] = static_cast<uint8_t>(mDeviceNameLength + 1); // length
    responseData[responseDataIndex++] = CHIP_ADV_DATA_TYPE_NAME;                     // AD type : name
    memcpy(&responseData[responseDataIndex], mDeviceName, mDeviceNameLength);        // AD value
    responseDataIndex += mDeviceNameLength;

    err = SilabsSetAdvertiserData(advDataIndex, (uint8_t *) advData, responseDataIndex, (uint8_t *) responseData);

exit:
    return err;
}

CHIP_ERROR BleManagerAbstraction::StartAdvertising(void)
{
    CHIP_ERROR err        = CHIP_NO_ERROR;
    uint32_t interval_min = 0;
    uint32_t interval_max = 0;

    // If already advertising, stop it, before changing values
    if (mFlags.Has(Flags::kAdvertising))
    {
        SilabsStopAdvertising();
    }

    ChipLogDetail(DeviceLayer, "Start BLE advertissement");

    // Configure Random Address for BLE advertising
    SilabsConfigureRandomAddress();

    err = ConfigureAdvertisingData();
    SuccessOrExit(err);

    mFlags.Clear(Flags::kRestartAdvertising);

    if (mFlags.Has(Flags::kFastAdvertisingEnabled))
    {
        interval_min = CHIP_DEVICE_CONFIG_BLE_FAST_ADVERTISING_INTERVAL_MIN;
        interval_max = CHIP_DEVICE_CONFIG_BLE_FAST_ADVERTISING_INTERVAL_MAX;
    }
    else
    {
        interval_min = CHIP_DEVICE_CONFIG_BLE_SLOW_ADVERTISING_INTERVAL_MIN;
        interval_max = CHIP_DEVICE_CONFIG_BLE_SLOW_ADVERTISING_INTERVAL_MAX;
    }

    SilabsStartAdvertising(interval_min, interval_max);

    if (err == CHIP_NO_ERROR)
    {
        if (mFlags.Has(Flags::kFastAdvertisingEnabled))
        {
            StartBleAdvTimeoutTimer(CHIP_DEVICE_CONFIG_BLE_ADVERTISING_INTERVAL_CHANGE_TIME);
        }
        mFlags.Set(Flags::kAdvertising);
    }

exit:
    return err;
}

CHIP_ERROR BleManagerAbstraction::StopAdvertising(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    if (mFlags.Has(Flags::kAdvertising))
    {
        mFlags.Clear(Flags::kAdvertising).Clear(Flags::kRestartAdvertising);
        mFlags.Set(Flags::kFastAdvertisingEnabled, true);

        err = SilabsStopAdvertising();
        SuccessOrExit(err);

        CancelBleAdvTimeoutTimer();
    }

exit:
    return err;
}

void BleManagerAbstraction::UpdateMtu(sl_bt_msg_t * evt)
{
    CHIPoBLEConState * bleConnState = GetConnectionState(evt->data.evt_gatt_mtu_exchanged.connection);
    if (bleConnState != NULL)
    {
        // bleConnState->MTU is a 10-bit field inside a uint16_t.  We're
        // assigning to it from a uint16_t, and compilers warn about
        // possibly not fitting.  There's no way to suppress that warning
        // via explicit cast; we have to disable the warning around the
        // assignment.
        //
        // TODO: https://github.com/project-chip/connectedhomeip/issues/2569
        // tracks making this safe with a check or explaining why no check
        // is needed.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
        bleConnState->mtu = evt->data.evt_gatt_mtu_exchanged.mtu;
#pragma GCC diagnostic pop
        ;
    }
}

void BleManagerAbstraction::HandleBootEvent(void)
{
    mFlags.Set(Flags::kSilabsBLEStackInitialized);
    PlatformMgr().ScheduleWork(DriveBLEState, reinterpret_cast<intptr_t>(this));
}

void BleManagerAbstraction::HandleConnectEvent(sl_bt_msg_t * evt)
{
    sl_bt_evt_connection_opened_t * conn_evt = (sl_bt_evt_connection_opened_t *) &(evt->data);
    uint8_t connHandle                       = conn_evt->connection;
    uint8_t bondingHandle                    = conn_evt->bonding;

    ChipLogProgress(DeviceLayer, "Connect Event for handle : %d", connHandle);

    AddConnection(connHandle, bondingHandle);

    PlatformMgr().ScheduleWork(DriveBLEState, reinterpret_cast<intptr_t>(this));
}

void BleManagerAbstraction::HandleConnectionCloseEvent(sl_bt_msg_t * evt)
{
    sl_bt_evt_connection_closed_t * conn_evt = (sl_bt_evt_connection_closed_t *) &(evt->data);
    uint8_t connHandle                       = conn_evt->connection;

    ChipLogProgress(DeviceLayer, "Disconnect Event for handle : %d", connHandle);

    if (RemoveConnection(connHandle))
    {
        ChipDeviceEvent event;
        event.Type                          = DeviceEventType::kCHIPoBLEConnectionError;
        event.CHIPoBLEConnectionError.ConId = connHandle;

        switch (conn_evt->reason)
        {
        case SL_STATUS_BT_CTRL_REMOTE_USER_TERMINATED:
        case SL_STATUS_BT_CTRL_REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_LOW_RESOURCES:
        case SL_STATUS_BT_CTRL_REMOTE_POWERING_OFF:
            event.CHIPoBLEConnectionError.Reason = BLE_ERROR_REMOTE_DEVICE_DISCONNECTED;
            break;

        case SL_STATUS_BT_CTRL_CONNECTION_TERMINATED_BY_LOCAL_HOST:
            event.CHIPoBLEConnectionError.Reason = BLE_ERROR_APP_CLOSED_CONNECTION;
            break;

        default:
            event.CHIPoBLEConnectionError.Reason = BLE_ERROR_CHIPOBLE_PROTOCOL_ABORT;
            break;
        }

        ChipLogProgress(DeviceLayer, "BLE GATT connection closed (con %u, reason %u)", connHandle, conn_evt->reason);

        PlatformMgr().PostEventOrDie(&event);

        // Arrange to re-enable connectable advertising in case it was disabled due to the
        // maximum connection limit being reached.
        mFlags.Set(Flags::kRestartAdvertising);
        mFlags.Set(Flags::kFastAdvertisingEnabled);
        PlatformMgr().ScheduleWork(DriveBLEState, reinterpret_cast<intptr_t>(this));
    }
}

void BleManagerAbstraction::HandleWriteEvent(sl_bt_msg_t * evt)
{
    uint16_t attribute = evt->data.evt_gatt_server_user_write_request.characteristic;

    ChipLogProgress(DeviceLayer, "Char Write Req, char : %d", attribute);

    if (gattdb_CHIPoBLEChar_Rx == attribute)
    {
        HandleRXCharWrite(evt);
    }
}

void BleManagerAbstraction::HandleTXCharCCCDWrite(sl_bt_msg_t * evt)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    CHIPoBLEConState * bleConnState;
    bool isIndicationEnabled = false;
    ChipDeviceEvent event;

    bleConnState = GetConnectionState(evt->data.evt_gatt_server_user_write_request.connection);
    VerifyOrExit(bleConnState != NULL, err = CHIP_ERROR_NO_MEMORY);

    // Determine if the client is enabling or disabling notification/indication.
    isIndicationEnabled = (evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_indication);

    ChipLogProgress(DeviceLayer, "HandleTXcharCCCDWrite - Config Flags value : %d",
                    evt->data.evt_gatt_server_characteristic_status.client_config_flags);
    ChipLogProgress(DeviceLayer, "CHIPoBLE %s received", isIndicationEnabled ? "subscribe" : "unsubscribe");

    if (isIndicationEnabled)
    {
        // If indications are not already enabled for the connection...
        if (!bleConnState->subscribed)
        {
            bleConnState->subscribed = 1;
            // Post an event to the CHIP queue to process either a CHIPoBLE Subscribe or Unsubscribe based on
            // whether the client is enabling or disabling indications.
            {
                event.Type                    = DeviceEventType::kCHIPoBLESubscribe;
                event.CHIPoBLESubscribe.ConId = evt->data.evt_gatt_server_user_write_request.connection;
                err                           = PlatformMgr().PostEvent(&event);
            }
        }
    }
    else
    {
        bleConnState->subscribed      = 0;
        event.Type                    = DeviceEventType::kCHIPoBLEUnsubscribe;
        event.CHIPoBLESubscribe.ConId = evt->data.evt_gatt_server_user_write_request.connection;
        err                           = PlatformMgr().PostEvent(&event);
    }

exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "HandleTXCharCCCDWrite() failed: %s", ErrorStr(err));
    }
}

void BleManagerAbstraction::HandleRXCharWrite(sl_bt_msg_t * evt)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    System::PacketBufferHandle buf;
    uint16_t writeLen = evt->data.evt_gatt_server_user_write_request.value.len;
    uint8_t * data    = (uint8_t *) evt->data.evt_gatt_server_user_write_request.value.data;

    // Copy the data to a packet buffer.
    buf = System::PacketBufferHandle::NewWithData(data, writeLen, 0, 0);
    VerifyOrExit(!buf.IsNull(), err = CHIP_ERROR_NO_MEMORY);

    ChipLogDetail(DeviceLayer, "Write request/command received for CHIPoBLE RX characteristic (con %u, len %u)",
                  evt->data.evt_gatt_server_user_write_request.connection, buf->DataLength());

    // Post an event to the CHIP queue to deliver the data into the CHIP stack.
    {
        ChipDeviceEvent event;
        event.Type                        = DeviceEventType::kCHIPoBLEWriteReceived;
        event.CHIPoBLEWriteReceived.ConId = evt->data.evt_gatt_server_user_write_request.connection;
        event.CHIPoBLEWriteReceived.Data  = std::move(buf).UnsafeRelease();
        err                               = PlatformMgr().PostEvent(&event);
    }

exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "HandleRXCharWrite() failed: %s", ErrorStr(err));
    }
}

void BleManagerAbstraction::HandleTxConfirmationEvent(BLE_CONNECTION_OBJECT conId)
{
    ChipDeviceEvent event;
    ChipLogProgress(DeviceLayer, "Tx Confirmation Event Received - Stop Timer");

    CHIPoBLEConState * conState = GetConnectionState(conId);
    VerifyOrReturn(conState != NULL);

    SystemLayer().CancelTimer(HandleSoftTimerEvent, conState);

    event.Type                          = DeviceEventType::kCHIPoBLEIndicateConfirm;
    event.CHIPoBLEIndicateConfirm.ConId = conId;
    PlatformMgr().PostEventOrDie(&event);
}

void BleManagerAbstraction::HandleSoftTimerEvent(System::Layer * apSystemLayer, void * apAppState)
{
    VerifyOrReturn(apAppState != NULL);
    CHIPoBLEConState * conState = static_cast<CHIPoBLEConState *>(apAppState);

    // Verify that the connection state is still allocated
    VerifyOrReturn(conState->allocated);

    ChipLogProgress(DeviceLayer, "BleManagerAbstraction::HandleSoftTimerEvent - CHIPOBLE_PROTOCOL_ABORT");
    ChipDeviceEvent event;
    event.Type                           = DeviceEventType::kCHIPoBLEConnectionError;
    event.CHIPoBLEConnectionError.ConId  = conState->connectionHandle;
    event.CHIPoBLEConnectionError.Reason = BLE_ERROR_CHIPOBLE_PROTOCOL_ABORT;
    PlatformMgr().PostEventOrDie(&event);
}

bool BleManagerAbstraction::RemoveConnection(uint8_t connectionHandle)
{
    CHIPoBLEConState * bleConnState = GetConnectionState(connectionHandle, true);
    bool status                     = false;

    // Cancel Timer before cleaning the pointer
    SystemLayer().CancelTimer(HandleSoftTimerEvent, bleConnState);

    if (bleConnState != NULL)
    {
        memset(bleConnState, 0, sizeof(CHIPoBLEConState));
        status = true;
    }

    return status;
}

void BleManagerAbstraction::AddConnection(uint8_t connectionHandle, uint8_t bondingHandle)
{
    CHIPoBLEConState * bleConnState = GetConnectionState(connectionHandle, true);

    if (bleConnState != NULL)
    {
        memset(bleConnState, 0, sizeof(CHIPoBLEConState));
        bleConnState->allocated        = 1;
        bleConnState->connectionHandle = connectionHandle;
        bleConnState->bondingHandle    = bondingHandle;
    }
}

BleManagerAbstraction::CHIPoBLEConState * BleManagerAbstraction::GetConnectionState(uint8_t connectionHandle, bool allocate)
{
    uint8_t freeIndex = kMaxConnections;

    for (uint8_t i = 0; i < kMaxConnections; i++)
    {
        if (mBleConnections[i].allocated == 1)
        {
            if (mBleConnections[i].connectionHandle == connectionHandle)
            {
                return &mBleConnections[i];
            }
        }

        else if (i < freeIndex)
        {
            freeIndex = i;
        }
    }

    if (allocate)
    {
        if (freeIndex < kMaxConnections)
        {
            return &mBleConnections[freeIndex];
        }

        ChipLogError(DeviceLayer, "Failed to allocate CHIPoBLEConState");
    }

    return NULL;
}

#if CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING
CHIP_ERROR BleManagerAbstraction::EncodeAdditionalDataTlv()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    BitFlags<AdditionalDataFields> additionalDataFields;
    AdditionalDataPayloadGeneratorParams additionalDataPayloadParams;

#if CHIP_ENABLE_ROTATING_DEVICE_ID && defined(CHIP_DEVICE_CONFIG_ROTATING_DEVICE_ID_UNIQUE_ID)
    uint8_t rotatingDeviceIdUniqueId[ConfigurationManager::kRotatingDeviceIDUniqueIDLength] = {};
    MutableByteSpan rotatingDeviceIdUniqueIdSpan(rotatingDeviceIdUniqueId);

    err = DeviceLayer::GetDeviceInstanceInfoProvider()->GetRotatingDeviceIdUniqueId(rotatingDeviceIdUniqueIdSpan);
    SuccessOrExit(err);
    err = ConfigurationMgr().GetLifetimeCounter(additionalDataPayloadParams.rotatingDeviceIdLifetimeCounter);
    SuccessOrExit(err);
    additionalDataPayloadParams.rotatingDeviceIdUniqueId = rotatingDeviceIdUniqueIdSpan;
    additionalDataFields.Set(AdditionalDataFields::RotatingDeviceId);
#endif /* CHIP_ENABLE_ROTATING_DEVICE_ID && defined(CHIP_DEVICE_CONFIG_ROTATING_DEVICE_ID_UNIQUE_ID) */

    err = AdditionalDataPayloadGenerator().generateAdditionalDataPayload(additionalDataPayloadParams, c3AdditionalDataBufferHandle,
                                                                         additionalDataFields);

exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Failed to generate TLV encoded Additional Data (%s)", __func__);
    }

    return err;
}

void BleManagerAbstraction::HandleC3ReadRequest(sl_bt_msg_t * evt)
{
    sl_bt_evt_gatt_server_user_read_request_t * readReq =
        (sl_bt_evt_gatt_server_user_read_request_t *) &(evt->data.evt_gatt_server_user_read_request);
    ChipLogDetail(DeviceLayer, "Read request received for CHIPoBLEChar_C3 - opcode:%d", readReq->att_opcode);
    sl_status_t ret = sl_bt_gatt_server_send_user_read_response(readReq->connection, readReq->characteristic, 0,
                                                                this->c3AdditionalDataBufferHandle->DataLength(),
                                                                this->c3AdditionalDataBufferHandle->Start(), nullptr);

    if (ret != SL_STATUS_OK)
    {
        ChipLogDetail(DeviceLayer, "Failed to send read response, err:%ld", ret);
    }
}
#endif // CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING

void BleManagerAbstraction::BleAdvTimeoutHandler(TimerHandle_t xTimer)
{
    if (BLEMgrImpl().mFlags.Has(Flags::kFastAdvertisingEnabled))
    {
        ChipLogDetail(DeviceLayer, "bleAdv Timeout : Start slow advertissment");
        BLEMgr().SetAdvertisingMode(BLEAdvertisingMode::kSlowAdvertising);
    }
}

void BleManagerAbstraction::CancelBleAdvTimeoutTimer(void)
{
    if (xTimerStop(sbleAdvTimeoutTimer, pdMS_TO_TICKS(0)) == pdFAIL)
    {
        ChipLogError(DeviceLayer, "Failed to stop BledAdv timeout timer");
    }
}

void BleManagerAbstraction::StartBleAdvTimeoutTimer(uint32_t aTimeoutInMs)
{
    if (xTimerIsTimerActive(sbleAdvTimeoutTimer))
    {
        CancelBleAdvTimeoutTimer();
    }

    // timer is not active, change its period to required value (== restart).
    // FreeRTOS- Block for a maximum of 100 ticks if the change period command
    // cannot immediately be sent to the timer command queue.
    if (xTimerChangePeriod(sbleAdvTimeoutTimer, pdMS_TO_TICKS(aTimeoutInMs), pdMS_TO_TICKS(100)) != pdPASS)
    {
        ChipLogError(DeviceLayer, "Failed to start BledAdv timeout timer");
    }
}

void BleManagerAbstraction::DriveBLEState(intptr_t arg)
{
    BleManagerAbstraction * bleManagerAbstraction = reinterpret_cast<BleManagerAbstraction *>(arg);
    bleManagerAbstraction->DriveBLEState();
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

#endif // CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
