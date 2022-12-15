# Silicon Labs Matter

The Matter protocol leverages existing IP technologies, including Wi-Fi and Thread, to build a unified wireless connectivity ecosystem for smart homes. Internet Protocol (IP)-based networking provides manufacturers with simplified development while improving device compatibility for consumers.

This release is certifiable to the Connectivity Standards Alliance (CSA)'s Matter 1.0 certification program (https://csa-iot.org/) .

Silicon Labs supports Matter on both 802.15.4 (Thread) and 802.11 (Wi-Fi) transport protocols. The Thread development use case differs from Wi-Fi because the Thread protocol requires the use of an OpenThread Border Router (OTBR).

The Unify Matter Bridge is an application that makes legacy devices, such as
Z-Wave and Zigbee devices, accessible on a Matter fabric. It does so by acting as
an _IoT Service_ in a Unify Framework. 

![Silicon Labs Matter Summary](./resources/silicon_labs_matter.png)

These pages contain all of the information required to use the Matter demos or start development with Matter on the Thread and Wi-Fi transport protocols. Silicon Labs offers two Matter development paths, both of which contain demos, examples, and all the code needed for Matter Accessory Device development on both Thread and Wi-Fi: 


- Within Simplicity Studio through the **Silicon Labs Matter GSDK Extension**, which can be downloaded inside Simplicity Studio alongside the Silicon Labs GSDK release
- On GitHub through the [**Silicon Labs Matter GitHub repo**](https://github.com/SiliconLabs/matter#readme)

**To see release notes** containing list of features and knowns issues go to [Matter Release Notes on Silicon Labs Matter GitHub](https://github.com/SiliconLabs/matter/releases/tag/v1.0.1-1.0).

**If you are new to Matter** or would like more information about Silicon Labs Matter-based products, see the [Matter content on silabs.com](https://www.silabs.com/wireless/matter). 

**For background information on the Matter standard**, see the [Connectivity Standard Alliance page](https://csa-iot.org/all-solutions/matter/).

**To get started with development**, see the [Getting Started Page](/matter/<docspace-docleaf-version>/matter-overview).

**For more detailed development information** on both Matter over Thread and Matter over Wi-Fi, see the [Matter Developer's Guide](/matter/<docspace-docleaf-version>/matter-developers-guide-overview).

The pages in the Getting Started and Developer's Guide sections are tagged to show whether the 
content applies to the GitHub and/or the Studio developer flows.
