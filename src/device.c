// SPDX-License-Identifier: GPL-2.0-only

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <libusb-1.0/libusb.h>

#include <kibo/device.h> // for deviceInfo

static char* get_value(char *line)
{
	char *p = strchr(line, '=');

	if (!p)
		p = strchr(line, ':');

	if (!p)
		return NULL;

	p++; // skip '=' or ':'

	// skip whitespace (spaces or tabs)
	while (*p && isspace((unsigned char)*p))
		p++;

	return p;
}

int read_resp(libusb_device_handle* dev, unsigned char *buffer, size_t size)
{
	int ret, transferred;

	ret = libusb_bulk_transfer(dev,
				   FB_BULK_IN_ENDPOINT,
				   buffer,
				   size,
				   &transferred,
				   1000);

	if (ret != 0)
	{
		printf("-: Read failed: %d\n", ret);
		return -1;
	}

	if (transferred >= 4 && strncmp((char*)buffer, "OKAY", 4) == 0)
		return 0;  // signal end

	return transferred;
}

deviceInfo get_devinfo(libusb_device_handle* dev)
{
	int ret = 0, tsfd = 0;
	unsigned char devinfo_cmd[9] = "oem info";
	const unsigned int cmd_len = 9;
	unsigned char responses[64][128];
	deviceInfo di = {};
	char *val;

	// to get devinfo we send "oem:info" then parse it

	ret = libusb_bulk_transfer(dev, FB_BULK_OUT_ENDPOINT, devinfo_cmd, cmd_len, &tsfd, 1000);
	if (!(ret == 0 && tsfd == cmd_len))
	{
		printf("kibo: Failed to write to device.\n\tDebug: %x != %x\n", tsfd, cmd_len);
		return di;
	}

	// read reply
	for (int i = 0; i < 64; i++)
	{
		int len = read_resp(dev, responses[i], sizeof(responses[i]));

		if (len <= 0)
			break;
	}

	val = get_value((char*)responses[2]); // Primary BC
	if (val) {
		if (strcmp(val, "ACT575") == 0)
			di.bcVer = ACT575;
		else if (strcmp(val, "ACQ160") == 0)
			di.bcVer = ACQ160;
	}

	val = get_value((char*)responses[6]); // Alt BC
	if (val) {
		if (strcmp(val, "ACT575") == 0)
			di.bcVer_alt = ACT575;
		else if (strcmp(val, "ACQ160") == 0)
			di.bcVer_alt = ACQ160;
	}

	val = get_value((char*)responses[9]); // SecureBoot status
	if (val) {
		if (strcmp(val, "false") == 0)
			di.secure = true;
		else
			di.secure = false;
	}

	val = get_value((char*)responses[13]); // Model
	if (val) {
		if (strcmp(val, "bbf100") == 0)
			di.dev = ATHENA;
		else if (strcmp(val, "bbe100") == 0)
			di.dev = LUNA;
	}

	return di;
}

bool device_supports_kibo(deviceInfo* di)
{
	/*
	 * We don't need to check device model as it should be
	 * impossible for luna to boot athena fw and vice-versa
	 */
	if (di->bcVer == di->bcVer_alt && di->bcVer != BC_INVALID)
		return true;

	return false;
}

const char* bootchain_to_string(enum bootchainVersion v)
{
	switch (v)
	{
		case ACQ160: return "ACQ160";
		case ACT575: return "ACT575";
		default:     return "INVALID";
	}
}

const char* device_to_string(enum deviceVariant d)
{
	switch (d)
	{
		case ATHENA: return "ATHENA";
		case LUNA:   return "LUNA";
		default:     return "INVALID";
	}
}

void do_devinfo(libusb_device_handle* dev)
{
	deviceInfo di = get_devinfo(dev);

	printf("Device info:\n");
	printf("  Model           : %s\n", device_to_string(di.dev));
	printf("  Primary BC      : %s\n", bootchain_to_string(di.bcVer));
	printf("  Backup BC       : %s\n", bootchain_to_string(di.bcVer_alt));
	printf("  Secure Boot     : %s\n", di.secure ? "true" : "false");

	if (di.bcVer == di.bcVer_alt && di.bcVer != BC_INVALID)
		printf("  Your device is vulnerable.\n");
	else
	{
		char target[7];

		if (di.dev == LUNA)
			strncpy(target, "ACT575", 7);
		if (di.dev == ATHENA)
			strncpy(target, "ACQ160", 7);

		printf("\n\tIncorrect version. Please install %s twice.\n", target);
	}
}
