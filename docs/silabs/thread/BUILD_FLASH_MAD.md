# How to Build and Flash the Matter Accessory Device (MAD)

The Matter Accessory Device, such as the lighting-app, is the actual Matter
device that you will commission onto the Matter network and control using the
chip-tool.

## Step 1: Get the Image File to Flash the MAD

We have provided three ways to get the required image to flash the MAD. You can
use one of the following options:

1. Use the pre-built image file available in both **[SIMPLICITY STUDIO] & [GITHUB]**
2. Build the image file from the '`matter`' repository
3. Build the image file from within Simplicity Studio using the Silicon Labs Matter GSDK Extension

    > **IMPORTANT:** A complete list of software requirements for Silicon Labs
    > Matter 15.4 development is included on the
    > [Matter Software Requirements](../prerequisites/SOFTWARE_REQUIREMENTS.md) page.
    > Be sure that you have satisfied these requirements before proceeding.
-   ### **Using the Pre-Built Image File**

    **[GITHUB]**

    All of the Matter Accessory Device image files are accessible through the
    [Matter Artifacts Page](../prerequisites/ARTIFACTS.md). If you are using a
    pre-built image file, you can skip forward to Step #2: Flashing the MAD.

    **[SIMPLICITY STUDIO]**

    All of the pre-built Matter Accessory Device (MAD) image files are available
    inside Simplicity Studio as Matter Demos. Once you have loaded the Matter Extension
    to the Gecko SDK (GSDK) you can access all of the associated pre-built image files. For more
    information on using Simplicity Studio Demos please look at the [Simplicity Studio User's Guide](https://docs.silabs.com/simplicity-studio-5-users-guide/latest)

 

-   ### **Building the Matter Image File from the Repository**

    **[GITHUB]**

    The Matter Accessory Device (lighting-app) can be built out of the [Silicon Labs Matter Github repo](https://github.com/SiliconLabs/matter).

    Documentation on how to build and use the lighting-app Matter Accessory
    Device is provided in the [Silicon Labs Matter Github Repo](https://github.com/SiliconLabs/matter) under `/examples/lighting-app/efr32`

    {*[Matter Lighting App Documentation](../../../examples/lighting-app/efr32/README.md)*}

    Please note that you only need to build a single device for the demo such as
    the lighting-app. If you wish to build other examples such as the sleepy end
    device you are welcome to, but it is not necessary for the demo.

    > Additional examples are provided in [/examples](../../../examples/) directory,
    or [/silabs_examples](../../../silabs_examples/) (such as `onoff-plug-app`).

    The build process puts all image files in the following location:

    `<git location>/matter/out/<app name>/<board name>`

 
-   ### **Building the Matter Image File from within Simplicity Studio**

    **[SIMPLICITY STUDIO]**

    Silicon Labs offers a complete IDE and Matter Development tool set inside Simplicity Studio. For more information on using Simplicity Studio to build your Matter Accessory Device please consider the [Matter in Simplicity Studio](../dev/studio/index.md) section in this documentation.

## Step 2: Flash the Matter Accessory Device

**[SIMPLICITY STUDIO] & [GITHUB]**

For more information on how to flash your Silabs development platform see the
following instructions:
[How to Flash a Silicon Labs Device](../general/FLASH_SILABS_DEVICE.md)

Once your Matter Accessory Device has been flashed it should show a QR code on
the LCD. If no QR Code is present it may be that you need to add a bootloader to
your device. Bootloader images are provided on the
[Matter Artifacts page](../prerequisites/ARTIFACTS.md).

---

{*[Table of Contents](../README.md) | [Thread Demo](./DEMO_OVERVIEW.md) |
[Wi-Fi Demo](../wifi/DEMO_OVERVIEW.md)*}
