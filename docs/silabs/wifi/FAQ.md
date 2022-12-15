# Frequently Asked Questions and Troubleshooting for Matter over Wi-Fi

## Troubleshooting

 

### 1. Bluetooth connection fails when trying to commission the system through the chip-tool:

**Command leading to error:**

`$ out/standalone/chip-tool pairing ble-wifi 1122 mySSID myPassword 20202021 3840`

Where `mySSID` is **your AP's SSID** and `mypassword` is **your AP's password**.

 

**Error example:**

```log
[1659464425.856025][34818:34823] CHIP:DL: HandlePlatformSpecificBLEEvent 16386
[1659464425.856035][34818:34823] CHIP:IN: Clearing BLE pending packets.
[1659464425.856055][34818:34823] CHIP:IN: BleConnection Error: ../../examples/chip-tool/third_party/connectedhomeip/src/platform/Linux/bluez/Helper.cpp:1775: CHIP Error 0x000000AC: Internal error
```

> This error indicates that the Bluetooth connection between your system and
> laptop is failing. Follow the given procedure and then retry the chip-tool
> commissioning command.

 

**Procedure:**

1. Stop Bluetooth service:

    ```shell
    $ systemctl stop bluetooth.service
    ```

2. Wait 20 seconds
3. Restart Bluetooth service:

    ```shell
    $ sudo service bluetooth restart
    ```

4. Unblock Bluetooth service:

    ```shell
    $ rfkill unblock all
    ```

5. Enable Bluetooth service:

    ```shell
    $ sudo systemctl enable bluetooth
    ```

6. Issue the pairing command:

    ```shell
    $ out/standalone/chip-tool pairing ble-wifi 1122 mySSID mypassword 20202021 3840
    ```

    > Where `mySSID` is **your AP's SSID** and `mypassword` is **your AP's
    > password**.

  

### 2. Unsupported certificate format Error:

 

> When trying to commission the system, if an `Unsupported certificate format`
> error (example below) is encountered, follow the procedure stated below.

 

**Error example:**

```log
[1659631352.672826][5076:5076] CHIP:TOO: Run command failure: ../../examples/chip-tool/third_party/connectedhomeip/src/controller/CHIPDeviceController.cpp:1275: CHIP Error 0x00000050: Unsupported certificate format
```

 

**Procedure:**

-   Delete the existing certificates on your laptop with the following command
    run from the `/connectedhomeip` directory:

    ```shell
    $ /bin/rm /tmp/chip_*
    ```

-   Issue the commissioning command

 

### 3. WLAN connection fails from RS9116 during commissioning when channel 13 is selected on the AP:

 

The required channel becomes available for connection when the WLAN connection region is configured during compilation to one that supports the channel, such as for Japan for channel 13. 

In order to use the desired channel, before building, make sure the WLAN connection region is configured correctly by reviewing/modifying the following lines in 
**/examples/platform/efr32/rs911x/rsi_wlan_config.h**:
```c
//Make sure this is set to RSI_ENABLE
#define RSI_SET_REGION_SUPPORT RSI_ENABLE

// Note that the channels available for WLAN connection depend on the region selected
// Make sure this is set to 1 to configure from RSI_REGION_CODE value below
// 0: region configurations taken from beacon
// 1: region configurations taken from user
#define RSI_SET_REGION_FROM_USER_OR_BEACON 1

// 0 : Default Region domain
// 1 : US
// 2 : EUROPE
// 3 : JAPAN
#define RSI_REGION_CODE 3
```

 

### 4 . Incorrect firmware version

 

```shell
cd ./third_party/silabs/wiseconnect-wifi-bt-sdk/firmware
```

You will get appropriate firmware in the above mentioned path.

Note:

1.  How to check the current using firmware version?

    > You can find the currently using firmware version in the DUT log.

2.  How to check whether you are using correct firmware vesrsion or not?

    > Compare last 5 digits of fimware version mention in the above path with
    > the currently using firmware version.

 

### 5 . Apple Homepod associated failures

 

If there is an Apple Homepod on the network paired with a Thread device, and a commissioning failure is seen with error `3000001`:

> Either remove the Apple Homepod from the network, or unpair it from all Thread devices, before re-trying the commissioning.

 

### 6 . Commissioning failure at step 18

 

1. Verify router configuration specifically related to IPV6 SLAAC, Internet Group Management Protocol (IGMP) snooping.
2. Delete all the files of chip-tool /tmp folder. (`rm -rf /tmp/chip_*`)
3. After checking the router configuration, factory-reset your access point.

 

### 7. Commissioning failure at step 16

 

Verify the access point settings, SSID, PSK, secirity type, REGION, CHANNEL.

 

### 8 . Inconsistent logs

 

Verify external power is supplied to rs911x

 

### 9 . To enable different security options on AP/Router 

 

1. Get the router address by entering `route -n` or ifconfig of ipconfig.
2. Enter the router address in the browser and enter the appropriate username and password.
3. Select the appropriate band.
4. In security, select type (WPA / WPA2 / WPA3). 

 

---

{*[Table of Contents](../README.md) | [Thread Demo](../thread/DEMO_OVERVIEW.md) |
[Wi-Fi Demo](./DEMO_OVERVIEW.md)*}
