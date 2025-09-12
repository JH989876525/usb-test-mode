/**
 * Copyright (c) 2023 innodisk Crop.
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
#define MAX_CTL_XFER	64
#define MAX_BULK_XFER	512
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

int showID(int cnt){
	printf("There are %d devices avavlible\n", cnt);
	for (int devid = 0; devid < cnt; devid++)
	{
		libusb_device *dev = session.dev[devid];
		libusb_get_device_descriptor(dev, &desc);
		printf("\tDevice %d : %04x:%04x\n", devid, desc.idVendor,desc.idProduct);
		printf("\t\tbus:%2u,address:%2u,protocal%2u\n", libusb_get_bus_number(dev), libusb_get_device_address(dev), desc.bDeviceProtocol);
	}
}

int main(int argc, char **argv)
{
	int port;
	unsigned int product_id = 0, vendor_id = 0, device_num = 0;
	int ret, count, test_mode, test_port;
	unsigned char *data = 0;
	unsigned int timeout_set = 50000000;
	ssize_t cnt;
	uint8_t bmRequestType = 0x23, bRequest = 0x03;
	uint16_t wValue, wLength;
	int opt;
	bool auto_mode = 0;

	if (geteuid() != 0) {
    	printf("Root in need, retry again with \"sudo\".\n");
    	return 0;
	}

	while ((opt = getopt(argc, argv, ":a")) != -1) {
        switch (opt) {
            case 'a':
                auto_mode = 1;
				printf("Auto mode enabled.\n");
                break;
            default:
        }
    }

	/* open context */
	ret = libusb_init(&session.ctx);
	if (ret < 0) {
		printf("Init Error %i occourred.\n", ret);
		return -EIO;
	}

	/* check numbers of device*/
	cnt = libusb_get_device_list(session.ctx, &session.dev);
	if (cnt < 0) {
		printf("no device found\n");
		libusb_exit(session.ctx);
		return -ENODEV;
	}

	showID(cnt);

	printf("Please select the device for testing (0-%ld)\n", cnt - 1);
	ret = scanf("%d", &device_num);
	if (!ret || (device_num >= cnt -1) || (device_num < 0)) {
		printf("Please enter 4 digit hex only (len=%d, 0x%x)\n", ret, device_num);
		return 0;
	}

	/* Selecting usb2.0 test case */ 
	libusb_get_device_descriptor(session.dev[device_num], &desc);
	if (auto_mode)
	{
		/* Selecting default test_mode */ 
		if (desc.bDeviceProtocol != 3)
		{
			test_mode = Test_Packet;
			wValue = 0x0015;
			wLength = 0x0000;
		}
		else
		{
			test_mode = LINK_COMPLIANCE_MODE;
			wLength = 0x0000;
			wValue = 0x0005;
		}
	}
	else
	{
		if (desc.bDeviceProtocol != 3)
		{
			printf("Please select the USB 2.0 test mode for device %d\n", device_num);
			printf("\tPress '1' for Test_J\n");
			printf("\tPress '2' for Test_K\n");
			printf("\tPress '3' for Test_SE0_NAK\n");
			printf("\tPress '4' for Test_Packet\n");
			printf("\tPress '5' for Test_Force_Enable\n");
			ret = scanf("%d", &test_mode);

			if (!ret || test_mode < TEST_J || test_mode > TEST_FORCE_ENABLE) {
				printf ("Test Mode %d is not avaiable\n", test_mode);
				return 0;
			}
			wValue = 0x0015;
			wLength = 0x0000;
		}
		else
		{
			printf("USB 3.0 test mode %d for device\n", LINK_COMPLIANCE_MODE);
			test_mode = LINK_COMPLIANCE_MODE;
			wLength = 0x0000;
			wValue = 0x0005;
		}

		/* selecting test port */ 
		printf("Please enter Test For USB Port Number (1-8):\n");
		ret = scanf("%d", &test_port);
		if (!ret || test_port < 1 || test_port > 8) {
			printf ("Test Port %d is not avaiable\n", test_port);
			return 0;
		}

		libusb_get_device_descriptor(session.dev[device_num], &desc);
		vendor_id = desc.idVendor;
		product_id = desc.idProduct;
		/* open device w/ vendorID and productID */
		printf("Opening device ID %04x:%04x...", vendor_id, product_id);
		libusb_device_handle * handle;
		libusb_device *dev = session.dev[device_num];
		libusb_open(dev,&handle);
		if (!handle) {
			printf("failed/not in list\n");
			libusb_exit(session.ctx);
			return -ENODEV;
		}
		printf("ok\n");

		}
	}

	/* free the list, unref the devices in it */
	libusb_free_device_list(session.dev, 1);

	/* find out if a kernel driver is attached */
	if (libusb_kernel_driver_active(handle, 0) == 1) {
		printf("Device has kernel driver attached.\n");
		/* detach it */
		if (!libusb_detach_kernel_driver(handle, 0))
			printf("Kernel Driver Detached!\n");
	}

	/* Send Endpoint Reflector control transfer */
	if (auto_mode)
	{
		/* setup all devices */
		for (int i_test_port = 1; i_test_port <= 8; i_test_port++)
		{
			ret = libusb_control_transfer(handle,
								bmRequestType,
								bRequest,
								wValue,
								test_mode << 8 | i_test_port << 0,
								data,
								wLength,
								timeout_set);
			if (!ret)
				printf("Port %d now in test mode!\n", i_test_port);
			else
				printf("Port %d is not avalible, ret: %d\n", i_test_port, ret);
		}
		
	}
	else
	{
		ret = libusb_control_transfer(handle,
							bmRequestType,
							bRequest,
							wValue,
							test_mode << 8 | test_port << 0,
							data,
							wLength,
							timeout_set);
		if (!ret)
			printf("Port now in test mode!\n");
		else
			printf("Control transfer failed. Error: %d\n", ret);
	}

	/* close the device we opened */
	libusb_close(handle);
	libusb_close(session.dev_handle);
	libusb_exit(session.ctx);
	printf("Please Reset System for Next USB Test\n");
	return 0;
}
