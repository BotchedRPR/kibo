// SPDX-License-Identifier: GPL-2.0-only

#ifndef KIBO_DEVICE_H
#define KIBO_DEVICE_H

#include <stdbool.h>

#include <libusb-1.0/libusb.h>

#define FB_BULK_IN_ENDPOINT	0x81
#define FB_BULK_OUT_ENDPOINT	0x01

enum bootchainVersion {
	BC_INVALID,
	ACA360,
	ACQ160,
	ACT575,
};

enum deviceVariant {
	DEV_INVALID,
	ATHENA,
	LUNA,
};

typedef struct deviceInfo {
	enum deviceVariant dev;
	enum bootchainVersion bcVer;
	enum bootchainVersion bcVer_alt;
	bool secure;
} deviceInfo;

deviceInfo get_devinfo(libusb_device_handle* dev);
bool device_supports_kibo(deviceInfo* di); // for internal usage, checks fw version
void do_devinfo(libusb_device_handle* dev); // for CLI, print out pretty details

#endif // KIBO_DEVICE_H
