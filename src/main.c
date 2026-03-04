// SPDX-License-Identifier: GPL-2.0-only

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#include <kibo/device.h>  // do_devinfo
#include <kibo/exploit.h> // do_cool_stuff
#include <kibo/main.h>    // command_t

#define BB_VID		0x0FCA
#define KEY2_PID	0x8040

static void print_banner(void)
{
	printf("kibo - BlackBerry SDM660/636 bootloader tools\n");
	printf("Copyright (c) 2026 BotchedRPR\n\n");
	printf("This project is licensed under the GNU GPL 2.0.\n");
	printf("You may not use this file except in compliance with the License.\n\n");
}

static void print_help(void)
{
	print_banner();

	printf("Usage:\n kibo [action]\n\nwhere action is one of:\n");
	printf("\t - help    - print this usage menu\n");
	printf("\t - devinfo - print current device info\n");
	printf("\t - unlock  - set bootmode to FACTORY_MODE\n");
	printf("\t - lock    - set bootmode to PRODUCT_MODE\n\n");
	printf("WARNING: The following commands may permanently device your device if misused.\n");
	printf("\t - proxy   - disable RTAS auth for next command\n");
	printf("\t - diffuse - disable BbryWipeLib functionality\n");
}

static command_t commands[] = {
	{"devinfo", do_devinfo, false},
	{"unlock",  do_unlock, true},
	{"lock",    do_lock, true},
	{"proxy",   do_rtas, true},
	{"diffuse", do_diffuse, true},
};

static libusb_context* init_context()
{
	libusb_context* ctx;
	int ret;

	ret = libusb_init_context(&ctx, NULL, 0);
	if (ret != 0)
	{
		printf("kibo: Failed to init libusb: %s\n", libusb_error_name(ret));
		return NULL;
	}

	libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);

	return ctx;
}

libusb_device_handle* getDevice(libusb_context* ctx)
{
	// snipped from https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
	libusb_device **list;
	libusb_device *found = NULL;
	libusb_device_handle *handle = NULL;
	ssize_t cnt = libusb_get_device_list(ctx, &list);
	ssize_t i = 0;
	int err = 0;

	if (cnt < 0)
	{
		printf("kibo: libusb error: Failed to get the device list\n");
		return NULL;
	}

	for (i = 0; i < cnt; i++)
	{
		libusb_device *device = list[i];
		struct libusb_device_descriptor desc;

		err = libusb_get_device_descriptor(device, &desc);
		if (err == 0 && desc.idVendor == BB_VID && desc.idProduct == KEY2_PID)
		{
			found = device;
			break;
		}
		else if (err != 0)
		{
			printf("kibo: libusb error: Failed to get a device descriptor: %s\n", libusb_error_name(err));
			return NULL;
		}
	}

	if (found)
	{
		err = libusb_open(found, &handle);

		if (err)
		{
			printf("kibo: libusb error: Failed to open device: %s\n", libusb_error_name(err));
			return NULL;
		}

		libusb_set_auto_detach_kernel_driver(handle, true);
		libusb_claim_interface(handle, 0);
	}

	libusb_free_device_list(list, 1);
	return handle;
}

int main(int argc, char* argv[])
{
	int ret = 0;
	libusb_context* ctx = NULL;
	libusb_device_handle* dev = NULL;

	if (argc < 2 || strcmp(argv[1], "help") == 0)
	{
		print_help();
		ret = 1;
		goto exit;
	}

	print_banner();

	// we need the context for... anything usb-related
	ctx = init_context();
	if (!ctx)
	{
		ret = 2;
		goto exit;
	}

	// we need the device handle for the commands
	dev = getDevice(ctx);
	if (!dev)
	{
		ret = 3;
		printf("kibo: Couldn't find device or failed to open it. Sorry.\n");
		goto exit;
	}

	for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
	{
		if (strcmp(argv[1], commands[i].name) == 0)
		{
			if (commands[i].isExploit)
			{
				printf("kibo: Rebooting device to clear the fastboot buffer...\n");
				prepare_for_exploit(dev);

				// after this the device will reboot, so we need to get it again
				libusb_close(dev);
				dev = NULL;

				// give it some time to GTFO
				sleep(3);

				printf("kibo: Waiting for device to reconnect...\n");

				while (dev == NULL)
				{
					dev = getDevice(ctx);
					sleep(2);
				}

				// and then we can move on to the real exploit.
			}

			commands[i].func(dev);
			goto exit;
		}
	}

	printf("kibo: Unknown action: %s\n", argv[1]);
	print_help();

exit:
	if (dev)
		libusb_close(dev);
	if (ctx)
		libusb_exit(ctx);
	return ret;
}
