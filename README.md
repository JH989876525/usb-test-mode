<!--
 Copyright (c) 2025 innodisk Crop.
 
 This software is released under the MIT License.
 https://opensource.org/licenses/MIT
-->

- [Overview](#overview)
- [Requirement](#requirement)
- [Build](#build)
- [Run](#run)
- [FAQ](#faq)

# Overview
A simple tool for USB2.0 & USB3.0 compliance test under linux OS.

# Requirement
- [libusb](https://libusb.info/)

# Build
```bash
make 
```

# Run
Reset the power of system between each test.
1. Using `lsusb` to check the USB topology of the system.
    ```bash
    lsusb -t
    ```
2. Run `USB_TEST_MODE`.
    ```bash
    sudo ./USB_TEST_MODE
    ```
3. Select the USB device number according to bus ID & device ID from `lsusb`.
   ```bash
   Bus 001 Device 002: ID 05e3:0610 Genesys Logic, Inc. Hub
   ```
4. Select the test mode if device is USB2.0.
5. Select which port to enter the test mode. The number of port can follow the datasheet of the USB device.
6. Done, now user can check the wavefrom for the compliance test. 

- `Auto mode` : Auto select test mode and enable all available port of device into compliance mode.
    ```bash
    sudo ./USB_TEST_MODE -a
    ```

# FAQ
- Which test case should I use.  
    Generally suggest using `Packet`.

- What if the function not working under linux OS.  
    Using `usb test` at u-boot, for example `usb test 1 1 P`
