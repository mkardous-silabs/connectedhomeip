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
#include "rail.h"
#include <platform/internal/CHIPDeviceLayerInternal.h>
extern "C" {
#include "sl_bluetooth.h"
}
#include "gatt_db.h"

/**********************************************************
 * Constant
 *********************************************************/

namespace {

// Default Connection  parameters
constexpr uint16_t kBleConfigMinInterval = 16; // Time = Value x 1.25 ms = 30ms
constexpr uint16_t kBleConfigMaxInterval = 80; // Time = Value x 1.25 ms = 100ms
constexpr uint16_t kBleConfigLatency     = 0;
constexpr uint16_t kBleConfigTimeout     = 100;    // Time = Value x 10 ms = 1s
constexpr uint16_t kBleConfigMinCeLength = 0;      // Leave to min value
constexpr uint16_t kBleConfigMaxCeLength = 0xFFFF; // Leave to max value

} // namespace

namespace chip {
namespace DeviceLayer {
namespace Internal {

// Singleton instance
BLEManagerImpl BLEManagerImpl::sInstance;

/**********************************************************
 * BLEManagerImpl Implementation
 *********************************************************/

CHIP_ERROR BleManagerAbstraction::SilabsInitBLEManager()
{
    // EFR32 Platform Does not require a platform init
    return CHIP_NO_ERROR;
}

CHIP_ERROR BLEManagerImpl::SilabsSendIndication(uint8_t connectionId, uint16_t characteristicId, size_t length, uint8_t * data)
{
    sl_status_t ret = sl_bt_gatt_server_send_indication(connectionId, characteristicId, length, data);
    return MapBLEError(ret);
}

CHIP_ERROR BLEManagerImpl::SilabsSetAdvertiserData(size_t advDataLength, uint8_t * advData, size_t responseDataLength,
                                                   uint8_t * responseData)
{
    sl_status_t ret = SL_STATUS_OK;

    // Verify that we have not already any advertising data
    if (advertising_set_handle != 0xff)
    {
        // Clear existing advertiser data
        ret = sl_bt_advertiser_delete_set(advertising_set_handle);
        if (ret != SL_STATUS_OK)
        {
            CHIP_ERROR error = MapBLEError(ret);
            ChipLogError(DeviceLayer, "sl_bt_advertiser_delete_set() - Delete Adv Set failed: %s", ErrorStr(error));

            return error;
        }

        advertising_set_handle = 0xff;
    }

    // Create a new set of advertiser data
    ret = sl_bt_advertiser_create_set(&advertising_set_handle);
    if (ret != SL_STATUS_OK)
    {
        CHIP_ERROR error = MapBLEError(ret);
        ChipLogError(DeviceLayer, "sl_bt_advertiser_create_set() failed: %s", ErrorStr(error));

        return error;
    }

    // Set Advertising Data
    ret =
        sl_bt_legacy_advertiser_set_data(advertising_set_handle, sl_bt_advertiser_advertising_data_packet, advDataLength, advData);
    if (ret != SL_STATUS_OK)
    {
        CHIP_ERROR error = MapBLEError(ret);
        ChipLogError(DeviceLayer, "sl_bt_legacy_advertiser_set_data() - Advertising Data failed: %s", ErrorStr(error));

        return error;
    }

    ret = sl_bt_legacy_advertiser_set_data(advertising_set_handle, sl_bt_advertiser_scan_response_packet, responseDataLength,
                                           responseData);
    if (ret != SL_STATUS_OK)
    {
        CHIP_ERROR error = MapBLEError(ret);
        ChipLogError(DeviceLayer, "sl_bt_legacy_advertiser_set_data() - Scan Response failed: %s", ErrorStr(error));

        return error;
    }

    return MapBLEError(ret);
}

CHIP_ERROR BLEManagerImpl::SilabsConfigureRandomAddress()
{
    const uint8_t kResolvableRandomAddrType = 2; // Private resolvable random address type
    bd_addr unusedBdAddr;                        // We can ignore this field when setting random address.
    sl_bt_advertiser_set_random_address(advertising_set_handle, kResolvableRandomAddrType, unusedBdAddr, &unusedBdAddr);
    (void) unusedBdAddr;

    return CHIP_NO_ERROR;
}

CHIP_ERROR BLEManagerImpl::SilabsStartAdvertising(uint32_t minInterval, uint32_t maxInterval)
{
    sl_status_t ret = SL_STATUS_OK;

    uint16_t numConnections = NumConnections();
    uint8_t connectableAdv =
        (numConnections < kMaxConnections) ? sl_bt_advertiser_connectable_scannable : sl_bt_advertiser_scannable_non_connectable;

    // Set Advertising timings
    ret = sl_bt_advertiser_set_timing(advertising_set_handle, minInterval, maxInterval, 0, 0);
    if (ret != SL_STATUS_OK)
    {
        CHIP_ERROR error = MapBLEError(ret);
        ChipLogError(DeviceLayer, "sl_bt_advertiser_set_timing() failed: %s", ErrorStr(error));

        return error;
    }

    // Configure Advertiser
    sl_bt_advertiser_configure(advertising_set_handle, 1);

    // Start Advertising
    ret = sl_bt_legacy_advertiser_start(advertising_set_handle, connectableAdv);
    if (ret != SL_STATUS_OK)
    {
        CHIP_ERROR error = MapBLEError(ret);
        ChipLogError(DeviceLayer, "sl_bt_legacy_advertiser_start() failed: %s", ErrorStr(error));

        return error;
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR BLEManagerImpl::SilabsStopAdvertising()
{
    sl_status_t ret  = sl_bt_advertiser_stop(advertising_set_handle);
    CHIP_ERROR error = MapBLEError(ret);

    if (error != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "sl_bt_advertiser_set_timing() failed: %s", ErrorStr(error));
    }

    return error;
}

CHIP_ERROR BLEManagerImpl::SilabsConnectionClose(BLE_CONNECTION_OBJECT conId)
{
    sl_status_t ret  = sl_bt_connection_close(conId);
    CHIP_ERROR error = MapBLEError(ret);

    if (error != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "sl_bt_connection_close() failed: %s", ErrorStr(error));
    }

    return error;
}

CHIP_ERROR BLEManagerImpl::MapBLEError(int bleErr)
{
    switch (bleErr)
    {
    case SL_STATUS_OK:
        return CHIP_NO_ERROR;
    case SL_STATUS_BT_ATT_INVALID_ATT_LENGTH:
        return CHIP_ERROR_INVALID_STRING_LENGTH;
    case SL_STATUS_INVALID_PARAMETER:
        return CHIP_ERROR_INVALID_ARGUMENT;
    case SL_STATUS_INVALID_STATE:
        return CHIP_ERROR_INCORRECT_STATE;
    case SL_STATUS_NOT_SUPPORTED:
        return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
    default:
        return CHIP_ERROR(ChipError::Range::kPlatform, bleErr + CHIP_DEVICE_CONFIG_SILABS_BLE_ERROR_MIN);
    }
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

/**********************************************************
 * Gecko SDK Bluetooth Callback
 *********************************************************/

extern "C" void sl_bt_on_event(sl_bt_msg_t * evt)
{
    // As this is running in a separate thread, we need to block CHIP from operating,
    // until the events are handled.
    chip::DeviceLayer::PlatformMgr().LockChipStack();

    // handle bluetooth events
    switch (SL_BT_MSG_ID(evt->header))
    {
    case sl_bt_evt_system_boot_id: {
        ChipLogProgress(DeviceLayer, "Bluetooth stack booted: v%d.%d.%d-b%d", evt->data.evt_system_boot.major,
                        evt->data.evt_system_boot.minor, evt->data.evt_system_boot.patch, evt->data.evt_system_boot.build);
        chip::DeviceLayer::Internal::BLEMgrImpl().HandleBootEvent();

        RAIL_Version_t railVer;
        RAIL_GetVersion(&railVer, true);
        ChipLogProgress(DeviceLayer, "RAIL version:, v%d.%d.%d-b%d", railVer.major, railVer.minor, railVer.rev, railVer.build);
        sl_bt_connection_set_default_parameters(kBleConfigMinInterval, kBleConfigMaxInterval, kBleConfigLatency, kBleConfigTimeout,
                                                kBleConfigMinCeLength, kBleConfigMaxCeLength);
    }
    break;

    case sl_bt_evt_connection_opened_id: {
        chip::DeviceLayer::Internal::BLEMgrImpl().HandleConnectEvent(evt);
    }
    break;
    case sl_bt_evt_connection_parameters_id: {
        // ChipLogProgress(DeviceLayer, "Connection parameter ID received");
    }
    break;
    case sl_bt_evt_connection_phy_status_id: {
        // ChipLogProgress(DeviceLayer, "PHY update procedure is completed");
    }
    break;
    case sl_bt_evt_connection_closed_id: {
        chip::DeviceLayer::Internal::BLEMgrImpl().HandleConnectionCloseEvent(evt);
    }
    break;

    /* This event indicates that a remote GATT client is attempting to write a value of an
     * attribute in to the local GATT database, where the attribute was defined in the GATT
     * XML firmware configuration file to have type="user".  */
    case sl_bt_evt_gatt_server_attribute_value_id: {
        chip::DeviceLayer::Internal::BLEMgrImpl().HandleWriteEvent(evt);
    }
    break;

    case sl_bt_evt_gatt_mtu_exchanged_id: {
        chip::DeviceLayer::Internal::BLEMgrImpl().UpdateMtu(evt);
    }
    break;

    // confirmation of indication received from remote GATT client
    case sl_bt_evt_gatt_server_characteristic_status_id: {
        sl_bt_gatt_server_characteristic_status_flag_t StatusFlags;

        StatusFlags = (sl_bt_gatt_server_characteristic_status_flag_t) evt->data.evt_gatt_server_characteristic_status.status_flags;

        if (sl_bt_gatt_server_confirmation == StatusFlags)
        {
            chip::DeviceLayer::Internal::BLEMgrImpl().HandleTxConfirmationEvent(
                evt->data.evt_gatt_server_characteristic_status.connection);
        }
        else if ((evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_CHIPoBLEChar_Tx) &&
                 (evt->data.evt_gatt_server_characteristic_status.status_flags == gatt_server_client_config))
        {
            chip::DeviceLayer::Internal::BLEMgrImpl().HandleTXCharCCCDWrite(evt);
        }
    }
    break;

    case sl_bt_evt_gatt_server_user_read_request_id: {
        ChipLogProgress(DeviceLayer, "GATT server user_read_request");
#if CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING
        if (evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_CHIPoBLEChar_C3)
        {
            chip::DeviceLayer::Internal::BLEMgrImpl().HandleC3ReadRequest(evt);
        }
#endif // CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING
    }
    break;

    case sl_bt_evt_connection_remote_used_features_id: {
        // ChipLogProgress(DeviceLayer, "link layer features supported by the remote device");
    }
    break;

    default:
        ChipLogProgress(DeviceLayer, "evt_UNKNOWN id = %08" PRIx32, SL_BT_MSG_ID(evt->header));
        break;
    }

    chip::DeviceLayer::PlatformMgr().UnlockChipStack();
}
