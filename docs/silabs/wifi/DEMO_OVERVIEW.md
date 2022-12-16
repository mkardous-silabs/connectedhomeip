# Matter over Wi-Fi Demo Overview

This document walks through the steps to build the Matter application i.e lighting, thermostat, door-lock for 
EFR32MG boards. A complete list of hardware supported for Wi-Fi is included on the [Hardware Requirements page](../prerequisites/HARDWARE_REQUIREMENTS.md).

## Step 1: Matter Wi-Fi Prerequisites

**[SIMPLICITY STUDIO] & [GITHUB]**

Before running the Matter Wi-Fi demo or developing for Wi-Fi make sure that you have all the required hardware and software for each
use case.

1. [Matter Hardware Requirements](../prerequisites/HARDWARE_REQUIREMENTS.md)
1. [Matter Software Requirements](../prerequisites/SOFTWARE_REQUIREMENTS.md)

## Step 2: Building the chip-tool for Wi-Fi

**[SIMPLICITY STUDIO] & [GITHUB]**

In order to run the Matter Wi-Fi demo you will need to run the chip-tool on one
of two platforms, either Linux/Mac or Raspberry Pi.

1. [Build Linux Environment](BUILD_CHIP_ENV.md)

2. [Build Raspberry-Pi Environment](BUILD_PI_ENV.md)

## Step 3: Building the Matter Accessory Device (MAD) for Wi-Fi

**[SIMPLICITY STUDIO] & [GITHUB]**

Follow the steps in [Software Setup](SW_SETUP.md)

Build commands are provided for the RS9116 adapter.

## Step 4: Running the Demo

**[SIMPLICITY STUDIO] & [GITHUB]**

[Running Matter Demo over Wi-Fi using Linux](RUN_DEMO.md)

This contains instructions to run the Matter Wi-Fi demo using chip-tool
running on a Linux Machine (either Laptop or Raspberry Pi) - follow this
after successfully executing the above steps.

---

{*[Table of Contents](../README.md) | [Thread Demo](../thread/DEMO_OVERVIEW.md) |
[Wi-Fi Demo](./DEMO_OVERVIEW.md)*}
