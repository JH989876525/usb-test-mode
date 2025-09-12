/**
 * Copyright (c) 2025 innodisk Crop.
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libusb.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

/* the buffer sizes can exceed the USB MTU */
#define MAX_CTL_XFER    64
#define MAX_BULK_XFER   512
#define LINK_COMPLIANCE_MODE 0x0A

/**
 * struct my_usb_device - struct that ties USB related stuff
 * @dev: pointer libusb devic
 * @dev_handle: device handle for USB devices
 * @ctx: context of the current session
 * @device_desc: structure that holds the device descriptor fields
 * @inbuf: buffer for USB IN communication
 * @outbut: buffer for USB OUT communication
 */

struct libusb_session {
    libusb_device **dev;
    libusb_device_handle *dev_handle;
    libusb_context *ctx;
    struct libusb_device_descriptor device_desc;
    unsigned char inbuf[MAX_CTL_XFER];
    unsigned char outbuf[MAX_BULK_XFER];
    uint16_t wIndex;
    int port_num;
};

static struct libusb_session session;
static struct libusb_device_descriptor desc;

enum {
    TEST_J = 1,
    TEST_K,
    TEST_SE0_NAK,
    TEST_PACKET,
    TEST_FORCE_ENABLE,
};

void print_device_list(ssize_t cnt) {
    printf("There are %ld devices available\n", cnt);
    for (int devid = 0; devid < cnt; devid++) {
        libusb_device *dev = session.dev[devid];
        libusb_get_device_descriptor(dev, &desc);
        printf("\tDevice %d : %04x:%04x\n",
               devid, desc.idVendor, desc.idProduct);
        printf("\t\tbus:%2u,address:%2u,protocol %2u\n",
               libusb_get_bus_number(dev),
               libusb_get_device_address(dev),
               desc.bDeviceProtocol);
    }
}

libusb_device_handle* open_usb_device(int device_num) {
    libusb_device *dev = session.dev[device_num];
    libusb_device_handle *handle = NULL;

    if (libusb_open(dev, &handle)) {
        printf("Failed to open device\n");
        return NULL;
    }
    printf("Device opened successfully.\n");

    if (libusb_kernel_driver_active(handle, 0) == 1) {
        printf("Device has kernel driver attached.\n");
        if (!libusb_detach_kernel_driver(handle, 0))
            printf("Kernel Driver Detached!\n");
    }
    return handle;
}

int set_usb_test_mode(libusb_device_handle *handle,
                      uint8_t bmRequestType,
                      uint8_t bRequest,
                      uint16_t wValue,
                      int test_mode,
                      int port,
                      unsigned char *data,
                      uint16_t wLength,
                      unsigned int timeout) 
{
    int ret = libusb_control_transfer(handle,
                                      bmRequestType,
                                      bRequest,
                                      wValue,
                                      (test_mode << 8) | port,
                                      data,
                                      wLength,
                                      timeout);
    if (!ret) {
        printf("Port %d now in test mode!\n", port);
    } else {
        printf("Port %d failed, ret: %d\n", port, ret);
    }
    return ret;
}

int main(int argc, char **argv) {
    int device_num = 0, test_mode = 0, test_port = 0;
    int ret, opt;
    bool auto_mode = false;

    uint8_t bmRequestType = 0x23, bRequest = 0x03;
    uint16_t wValue = 0, wLength = 0;
    unsigned int timeout_set = 50000000;
    unsigned char *data = NULL;

    if (geteuid() != 0) {
        printf("Root is required, retry with \"sudo\".\n");
        return 0;
    }

    while ((opt = getopt(argc, argv, ":a")) != -1) {
        if (opt == 'a') {
            auto_mode = true;
            printf("Auto mode enabled.\n");
        }
    }

    if (libusb_init(&session.ctx) < 0) {
        printf("Init Error.\n");
        return -EIO;
    }

    ssize_t cnt = libusb_get_device_list(session.ctx, &session.dev);
    if (cnt < 0) {
        printf("No device found\n");
        libusb_exit(session.ctx);
        return -ENODEV;
    }

    print_device_list(cnt);

    printf("Select the device for testing (0-%ld): ", cnt - 1);
    if (scanf("%d", &device_num) != 1 || device_num < 0 || device_num >= cnt) {
        printf("Invalid device selection.\n");
        return 0;
    }

    libusb_get_device_descriptor(session.dev[device_num], &desc);

    if (auto_mode) {
        if (desc.bDeviceProtocol != 3) {
            test_mode = TEST_PACKET;
            wValue = 0x0015;
        } else {
            test_mode = LINK_COMPLIANCE_MODE;
            wValue = 0x0005;
        }
    } else {
        if (desc.bDeviceProtocol != 3) {
            printf("Select USB 2.0 test mode:\n");
            printf("\t1: Test_J\n\t2: Test_K\n\t3: Test_SE0_NAK\n");
            printf("\t4: Test_Packet\n\t5: Test_Force_Enable\n");
            if (scanf("%d", &test_mode) != 1 ||
                test_mode < TEST_J || test_mode > TEST_FORCE_ENABLE) {
                printf("Invalid test mode.\n");
                return 0;
            }
            wValue = 0x0015;
        } else {
            printf("USB 3.0 compliance mode selected.\n");
            test_mode = LINK_COMPLIANCE_MODE;
            wValue = 0x0005;
        }

        printf("Enter USB Test Port (1-8): ");
        if (scanf("%d", &test_port) != 1 || test_port < 1 || test_port > 8) {
            printf("Invalid port.\n");
            return 0;
        }
    }

    libusb_device_handle *handle = open_usb_device(device_num);
    if (!handle) {
        libusb_free_device_list(session.dev, 1);
        libusb_exit(session.ctx);
        return -ENODEV;
    }

    if (auto_mode) {
        for (int i = 1; i <= 8; i++) {
            set_usb_test_mode(handle, bmRequestType, bRequest,
                              wValue, test_mode, i, data, wLength, timeout_set);
        }
    } else {
        set_usb_test_mode(handle, bmRequestType, bRequest,
                          wValue, test_mode, test_port, data, wLength, timeout_set);
    }

    libusb_close(handle);
    libusb_free_device_list(session.dev, 1);
    libusb_exit(session.ctx);

    printf("Please reset system for next USB test.\n");
    return 0;
}
