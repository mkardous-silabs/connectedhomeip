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

#ifndef SIWX_917
#include "rail.h"
#endif

#include <crypto/RandUtils.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "wfx_host_events.h"
#include "wfx_rsi.h"
#include "wfx_sl_ble_init.h"
#include <rsi_driver.h>
#include <rsi_utils.h>

#ifdef __cplusplus
}
#endif

extern uint16_t rsi_ble_measurement_hndl;
extern rsi_ble_event_conn_status_t conn_event_to_app;
extern sl_wfx_msg_t event_msg;

rsi_semaphore_handle_t sl_rs_ble_init_sem;
rsi_semaphore_handle_t sl_ble_event_sem;

namespace {

/* wfxRsi Task will use as its stack */
StackType_t wfxBLETaskStack[WFX_RSI_TASK_SZ] = { 0 };
StaticTask_t rsiBLETaskStruct;

} // namespace

namespace chip {
namespace DeviceLayer {
namespace Internal {

BLEManagerImpl BLEManagerImpl::sInstance;

CHIP_ERROR BleManagerAbstraction::SilabsInitBLEManager()
{
    // Init Platform Semaphores
    rsi_semaphore_create(&sl_rs_ble_init_sem, 0);
    rsi_semaphore_create(&sl_ble_event_sem, 0);

    return CHIP_NO_ERROR;
}

CHIP_ERROR BLEManagerImpl::SilabsConnectionClose(BLE_CONNECTION_OBJECT conId)
{
    // TODO - Implement Connection Close
    return CHIP_NO_ERROR;
}

CHIP_ERROR BLEManagerImpl::SilabsSendIndication(uint8_t connectionId, uint16_t characteristicId, size_t length, uint8_t * data)
{
    uint32_t ret = rsi_ble_indicate_value(event_msg.resp_enh_conn.dev_addr, event_msg.rsi_ble_measurement_hndl, length, data);
    return MapBLEError(ret);
}

// TODO: Need to add RSI BLE STATUS codes
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

CHIP_ERROR BLEManagerImpl::SilabsSetAdvertiserData(size_t advDataLength, uint8_t * advData, size_t responseDataLength,
                                                   uint8_t * responseData)
{

    // TODO Finish
    uint32_t ret = rsi_ble_set_advertise_data(advData, index);

    ret = rsi_ble_set_scan_response_data(responseData, responseDataLength);
}

CHIP_ERROR BLEManagerImpl::SilabsStartAdvertising(uint32_t minInterval, uint32_t maxInterval)
{
    // TODO - Update min and max interval 
    rsi_ble_req_adv_t ble_adv = { 0 };

    ble_adv.status = RSI_BLE_START_ADV;

    ble_adv.adv_type         = RSI_BLE_ADV_TYPE;
    ble_adv.filter_type      = RSI_BLE_ADV_FILTER_TYPE;
    ble_adv.direct_addr_type = RSI_BLE_ADV_DIR_ADDR_TYPE;
    rsi_ascii_dev_address_to_6bytes_rev(ble_adv.direct_addr, (int8_t *) RSI_BLE_ADV_DIR_ADDR);
    ble_adv.adv_int_min     = RSI_BLE_ADV_INT_MIN;
    ble_adv.adv_int_max     = RSI_BLE_ADV_INT_MAX;
    ble_adv.own_addr_type   = LE_RANDOM_ADDRESS;
    ble_adv.adv_channel_map = RSI_BLE_ADV_CHANNEL_MAP;

    uint32_t ret = rsi_ble_start_advertising_with_values(&ble_adv);
    return MapBLEError(ret);
}

CHIP_ERROR BLEManagerImpl::SilabsStopAdvertising()
{
    uint32_t ret     = rsi_ble_stop_advertising();
    CHIP_ERROR error = MapBLEError(ret);

    if (error != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "sl_bt_advertiser_set_timing() failed: %s", ErrorStr(error));
    }

    return error;
}

CHIP_ERROR BLEManagerImpl::SilabsConfigureRandomAddress()
{
    // TODO
}


} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

void sl_ble_init()
{
    uint8_t randomAddrBLE[6] = { 0 };
    uint64_t randomAddr      = chip::Crypto::GetRandU64();

    // registering the GAP callback functions
    rsi_ble_gap_register_callbacks(NULL, NULL, rsi_ble_on_disconnect_event, NULL, NULL, NULL, rsi_ble_on_enhance_conn_status_event,
                                   NULL, NULL, NULL);

    // registering the GATT call back functions
    rsi_ble_gatt_register_callbacks(NULL, NULL, NULL, NULL, NULL, NULL, NULL, rsi_ble_on_gatt_write_event, NULL, NULL, NULL,
                                    rsi_ble_on_mtu_event, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                    rsi_ble_on_event_indication_confirmation, NULL);

    //  Exchange of GATT info with BLE stack

    rsi_ble_add_matter_service();

    //  initializing the application events map
    rsi_ble_app_init_events();
    memcpy(randomAddrBLE, &randomAddr, 6);
    rsi_ble_set_random_address_with_value(randomAddrBLE);
    chip::DeviceLayer::Internal::BLEMgrImpl().HandleBootEvent();
}

void sl_ble_event_handling_task(void)
{
    int32_t event_id;

    //! This semaphore is waiting for wifi module initialization.
    rsi_semaphore_wait(&sl_rs_ble_init_sem, 0);

    // This function initialize BLE and start BLE advertisement.
    sl_ble_init();

    // Application event map
    while (1)
    {
        //! This semaphore is waiting for next ble event task
        rsi_semaphore_wait(&sl_ble_event_sem, 0);

        // checking for events list
        event_id = rsi_ble_app_get_event();
        switch (event_id)
        {
        case RSI_BLE_CONN_EVENT: {
            rsi_ble_app_clear_event(RSI_BLE_CONN_EVENT);
            BLEMgrImpl().HandleConnectEvent();
            // Requests the connection parameters change with the remote device
            rsi_ble_conn_params_update(event_msg.resp_enh_conn.dev_addr, BLE_MIN_CONNECTION_INTERVAL_MS,
                                       BLE_MAX_CONNECTION_INTERVAL_MS, BLE_SLAVE_LATENCY_MS, BLE_TIMEOUT_MS);
        }
        break;
        case RSI_BLE_DISCONN_EVENT: {
            // event invokes when disconnection was completed
            BLEMgrImpl().HandleConnectionCloseEvent(event_msg.reason);
            // clear the served event
            rsi_ble_app_clear_event(RSI_BLE_DISCONN_EVENT);
        }
        break;
        case RSI_BLE_MTU_EVENT: {
            // event invokes when write/notification events received
            BLEMgrImpl().UpdateMtu(event_msg.rsi_ble_mtu);
            // clear the served event
            rsi_ble_app_clear_event(RSI_BLE_MTU_EVENT);
        }
        break;
        case RSI_BLE_GATT_WRITE_EVENT: {
            // event invokes when write/notification events received
            BLEMgrImpl().HandleWriteEvent(event_msg.rsi_ble_write);
            // clear the served event
            rsi_ble_app_clear_event(RSI_BLE_GATT_WRITE_EVENT);
        }
        break;
        case RSI_BLE_GATT_INDICATION_CONFIRMATION: {
            BLEMgrImpl().HandleTxConfirmationEvent(1);
            rsi_ble_app_clear_event(RSI_BLE_GATT_INDICATION_CONFIRMATION);
        }
        break;
        default:
            break;
        }
    }
}
