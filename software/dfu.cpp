/* Copyright (c) 2015 Vasily Voropaev <vvg@cubitel.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include <stdio.h>
#include <libusb.h>

#include "dfu.h"


#define STM32_DFU_VID	0x0483
#define STM32_DFU_PID	0xDF11

static libusb_device_handle *dev = NULL;


int dfuOpen()
{
	dev = libusb_open_device_with_vid_pid(NULL, STM32_DFU_VID, STM32_DFU_PID);
	if (!dev) return 0;

	return 1;
}

int dfuDownload(char *filename)
{
	FILE *f;
	size_t fsize;
	uint8_t *data;

	/* Check if device is opened */
	if (!dev) return 0;

	/* Read firmware file */

	f = fopen(filename, "rb");
	if (!f) return 0;

	fsize = fseek(f, 0, SEEK_END);
	fseek(f, 0, SEEK_SET);

	data = (uint8_t *)malloc(fsize);
	if (!data) {
		fclose(f);
		return 0;
	}

	fread(data, 1, fsize, f);

	fclose(f);

	/* Free memory */
	free(data);

	return 1;
}

void dfuClose()
{
	libusb_close(dev);
}
