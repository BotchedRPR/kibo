// SPDX-License-Identifier: GPL-2.0-only

#ifndef KIBO_MAIN_H
#define KIBO_MAIN_H

#include <stdbool.h>

#include <libusb-1.0/libusb.h>

#ifdef _WIN32
#include <windows.h>
#define msleep(ms) Sleep(ms)
#else
#include <unistd.h>
#define msleep(ms) usleep((ms) * 1000)
#endif

typedef void (*cmd_func)(libusb_device_handle* dev);

typedef struct {
	const char *name;
	cmd_func func;
	bool isExploit;
} command_t;

libusb_device_handle* getDevice(libusb_context* ctx);

#endif // KIBO_MAIN_H
