/**
 * Copyright (c) 2025 innodisk Crop.
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>

#define MAX_CTL_XFER    64
#define LINK_COMPLIANCE_MODE 0x0A

// USB 2.0 Test Modes
enum {
    TEST_J = 1,
    TEST_K,
    TEST_SE0_NAK,
    TEST_PACKET,
    TEST_FORCE_ENABLE,
};

typedef struct {
    libusb_device *dev;
    uint8_t bus;
    uint8_t addr;
    struct libusb_device_descriptor desc;
} DeviceInfo;

void print_usage(char *bin_name) {
    printf("Usage: sudo %s [OPTIONS]\n\n", bin_name);
    printf("Options:\n");
    printf("  -a, --auto      Enable auto mode (sets ports 8 to 1 automatically)\n");
    printf("  -h, --help      Display this help menu\n\n");
    printf("Description:\n");
    printf("  1. Lists USB devices sorted by Bus and Address.\n");
    printf("  2. In Auto Mode, USB 3.0 devices enter Compliance Mode,\n");
    printf("     and USB 2.0 devices enter Packet Test Mode.\n");
    printf("  3. Ports are initialized from High (8) to Low (1).\n");
}

/* --- Utility Functions --- */

// Comparison function for qsort: Sort by Bus (Asc), then Address (Asc)
int compare_devices(const void *a, const void *b) {
    DeviceInfo *devA = (DeviceInfo *)a;
    DeviceInfo *devB = (DeviceInfo *)b;
    
    if (devA->bus != devB->bus)
        return devA->bus - devB->bus;
    return devA->addr - devB->addr;
}

void cleanup(libusb_context *ctx, libusb_device **list, DeviceInfo *info) {
    if (info) free(info);
    if (list) libusb_free_device_list(list, 1);
    if (ctx) libusb_exit(ctx);
}

int set_usb_test_mode(libusb_device_handle *handle, int test_mode, int port, uint16_t wValue) {
    // Standard Hub/Port request constants
    uint8_t bmRequestType = 0x23; 
    uint8_t bRequest = 0x03;      // SET_FEATURE
    uint16_t wIndex = (uint16_t)((test_mode << 8) | port);
    
    int ret = libusb_control_transfer(handle, bmRequestType, bRequest, wValue, wIndex, NULL, 0, 5000);
    
    if (ret >= 0) {
        printf("  [OK] Port %d entered test mode 0x%02X\n", port, test_mode);
    } else {
        fprintf(stderr, "  [FAIL] Port %d, Error: %s\n", port, libusb_strerror(ret));
    }
    return ret;
}

/* --- Core Logic --- */

int main(int argc, char **argv) {
    libusb_context *ctx = NULL;
    libusb_device **dev_list = NULL;
    DeviceInfo *devices = NULL;
    bool auto_mode = false;
    int opt;

    // Command line parsing
    static struct option long_options[] = {
        {"auto", no_argument, 0, 'a'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "ah", long_options, NULL)) != -1) {
        switch (opt) {
            case 'a': auto_mode = true; break;
            case 'h': print_usage(argv[0]); return 0;
            default:  print_usage(argv[0]); return 1;
        }
    }

    if (geteuid() != 0) {
        printf("Error: Root privileges required. Please use sudo.\n");
        return EPERM;
    }

    if (libusb_init(&ctx) < 0) return -EIO;

    ssize_t cnt = libusb_get_device_list(ctx, &dev_list);
    if (cnt <= 0) {
        printf("No USB devices found.\n");
        cleanup(ctx, dev_list, NULL);
        return 0;
    }

    // Prepare sortable device array
    devices = malloc(sizeof(DeviceInfo) * cnt);
    for (int i = 0; i < cnt; i++) {
        devices[i].dev = dev_list[i];
        devices[i].bus = libusb_get_bus_number(dev_list[i]);
        devices[i].addr = libusb_get_device_address(dev_list[i]);
        libusb_get_device_descriptor(dev_list[i], &devices[i].desc);
    }

    // Requirement 1: List devices by sequence of bus and address
    qsort(devices, cnt, sizeof(DeviceInfo), compare_devices);

    printf("\n%-4s %-10s %-10s %-15s %-10s\n", "ID", "Bus", "Address", "VID:PID", "Protocol");
    printf("---------------------------------------------------\n");
    for (int i = 0; i < cnt; i++) {
        printf("[%2d] Bus %03u    Addr %03u    %04x:%04x       USB %d.0\n", 
                i, devices[i].bus, devices[i].addr, 
                devices[i].desc.idVendor, devices[i].desc.idProduct,
                (devices[i].desc.bDeviceProtocol == 3) ? 3 : 2);
    }

    int selection;
    printf("\nSelect device ID (0-%ld): ", cnt - 1);
    if (scanf("%d", &selection) != 1 || selection < 0 || selection >= cnt) {
        cleanup(ctx, dev_list, devices);
        return EINVAL;
    }

    libusb_device_handle *handle = NULL;
    if (libusb_open(devices[selection].dev, &handle) != 0) {
        printf("Failed to open device.\n");
        cleanup(ctx, dev_list, devices);
        return EIO;
    }

    libusb_set_auto_detach_kernel_driver(handle, 1);

    int test_mode, target_port = 0;
    uint16_t wValue;

    // Determine Protocol and Mode
    bool is_usb3 = (devices[selection].desc.bDeviceProtocol == 3);
    if (is_usb3) {
        test_mode = LINK_COMPLIANCE_MODE;
        wValue = 0x0005; // PORT_LINK_STATE
    } else {
        wValue = 0x0015; // PORT_TEST_CONTROL
        if (auto_mode) {
            test_mode = TEST_PACKET;
        } else {
            printf("Select USB 2.0 Mode (1:J, 2:K, 3:SE0, 4:Packet, 5:Force): ");
            scanf("%d", &test_mode);
        }
    }

    // Requirement 2: Auto mode setup port high to low
    if (auto_mode) {
        printf("Auto-mode: Setting ports 8 down to 1...\n");
        for (int p = 8; p >= 1; p--) {
            set_usb_test_mode(handle, test_mode, p, wValue);
        }
    } else {
        printf("Enter Port (1-8): ");
        scanf("%d", &target_port);
        set_usb_test_mode(handle, test_mode, target_port, wValue);
    }

    libusb_close(handle);
    cleanup(ctx, dev_list, devices);
    printf("\nDone. Please reset the system to exit test mode.\n");
    return 0;
}