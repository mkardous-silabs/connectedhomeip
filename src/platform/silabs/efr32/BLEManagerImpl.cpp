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
#include <platform/internal/CHIPDeviceLayerInternal.h>
#include "rail.h"
extern "C" {
#include "sl_bluetooth.h"
}
#include "gatt_db.h"

namespace chip {
namespace DeviceLayer {
namespace Internal {

// Singleton instance
BLEManagerImpl BLEManagerImpl::sInstance;

/**********************************************************
 * Constant
 *********************************************************/

// Default Connection  parameters
static constexpr uint16_t kBleConfigMinInterval = 16; // Time = Value x 1.25 ms = 30ms
static constexpr uint16_t kBleConfigMaxInterval = 80; // Time = Value x 1.25 ms = 100ms
static constexpr uint16_t kBleConfigLatency     = 0;
static constexpr uint16_t kBleConfigTimeout     = 100;    // Time = Value x 10 ms = 1s
static constexpr uint16_t kBleConfigMinCeLength = 0;      // Leave to min value
static constexpr uint16_t kBleConfigMaxCeLength = 0xFFFF; // Leave to max value

/**********************************************************
 * BLEManagerImpl Implementation
 *********************************************************/

void BLEManagerImpl::ProofOfConcept()
{
    // stuff
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
        sl_bt_connection_set_default_parameters(
            chip::DeviceLayer::Internal::kBleConfigMinInterval, chip::DeviceLayer::Internal::kBleConfigMaxInterval,
            chip::DeviceLayer::Internal::kBleConfigLatency, chip::DeviceLayer::Internal::kBleConfigTimeout,
            chip::DeviceLayer::Internal::kBleConfigMinCeLength, chip::DeviceLayer::Internal::kBleConfigMaxCeLength);
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

    /* Software Timer event */
    case sl_bt_evt_system_soft_timer_id: {
        chip::DeviceLayer::Internal::BLEMgrImpl().HandleSoftTimerEvent(evt);
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
