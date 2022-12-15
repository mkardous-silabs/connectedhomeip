# Matter Software Artifacts

This page provides links to pre-built software image "artifacts" that can be
used to set up the Matter Demo for the Thread and Wi-Fi use cases. These artifacts can be used 
in both the **[SIMPLICITY STUDIO] & [GITHUB]** use cases.


## Matter Hub Raspberry Pi Image

**[SIMPLICITY STUDIO] & [GITHUB]**

The Matter Hub image is intended to be flashed onto an SD card for a Raspberry
Pi. The Matter Hub Image provides both an Open Thread Border Router and the
Matter chip-tool. Note the image is ~10GB in size so depending on your internet
connection this download may take some time. Start the Matter Hub Raspberry Pi
image download here:

https://www.silabs.com/documents/public/software/SilabsMatterPi.zip

 

## Radio Co-Processor (RCP) Images

**[GITHUB]**

The Radio Co-Processor firmware is used to turn an EFR into an RCP that can be
used with a Raspberry Pi to allow the Raspberry Pi's Open Thread Border Router
to access the Thread network. Radio Co-Processor (RCP) images are available in
the Assets section of this page:

https://github.com/SiliconLabs/matter/releases/tag/v1.0.1-1.0


**[SIMPLICITY STUDIO]**

It is possible to build the Radio Co-Processor of your choice from within Simplicity
Studio using the Thread stack in the Gecko SDK (GSDK). More information on creating an
RCP image in Simplicity Studio is provided in [AN1256](https://www.silabs.com/documents/public/application-notes/an1256-using-sl-rcp-with-openthread-border-router.pdf).

## Matter Accessory Device Images

**[GITHUB]**

The Matter Accessory Device Images are used to turn an EFR into a Matter device.
These are pre-built binary images for the Matter Demo. Matter Accessory Device
Images are located in the Assets section of this page:

https://github.com/SiliconLabs/matter/releases/tag/v1.0.1-1.0

For Matter over Thread, 3 different types of images are provided:

1. **Standard**: Includes all code including enabling the LCD for a QR Code that can be used for commissioning.
2. **Release**: A smaller image size with reduced functionality, including removal of the LCD support, thus no QR Code.
3. **Sleepy**: A sleepy device image for improved energy efficiency and running on battery power.

**[SIMPLICITY STUDIO]**

Matter Accessory Device Images are also provided as "Demos" in the Matter GSDK extension. For more information on
using MAD images in Simplicity Studio look at the [Simplicity Studio Documentation](../dev/studio/index.md) included in this reference. For more information on using Simplicity Studio and the Launcher look at the [Simplicity Studio Documentation](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/), available on docs.silabs.com.


## Matter Bootloader Binaries

**[SIMPLICITY STUDIO] & [GITHUB]**

If you are using the OTA functionality and especially if you are using an
EFR32MG2x device, you will need to flash a bootloader binary on your device along
with the application image. Bootloader binaries for all of the Matter supported
devices are available here:

https://github.com/SiliconLabs/matter/releases/tag/v1.0.1-1.0



## RS9116 Firmware

**[SIMPLICITY STUDIO] & [GITHUB]**

The RS9116 firmware is used to update the RS9116 - it can be found in the
repository you have cloned, at the following relative path from the `/matter`
directory, `./third_party/silabs/wiseconnect-wifi-bt-sdk/firmware`

 

## SiWx917 Firmware

**[SIMPLICITY STUDIO] & [GITHUB]**

SiWx917 Firmware(rps) images are available in
the Assets section of this page:

https://github.com/SiliconLabs/matter/releases/tag/v1.0.1-1.0
