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

#include <libusb.h>

#include "device.h"
#include "protocol.h"


#define AFM_VID				0x1209
#define AFM_PID				0x6742

#define AFM_BULK_IN			0x81

#define USB_EP0_TIMEOUT		500
#define USB_BULK_TIMEOUT	1000


Device::Device()
{
	afm = NULL;
	m_image = NULL;
	libusb_init(NULL);
}

int Device::AfmCommand(uint8_t cmd, uint8_t direction, uint8_t *data, int len)
{
	uint8_t type;
	int ret;

	if (!afm) return 0;

	type = LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE |
			((direction == DEVICE_GET) ? LIBUSB_ENDPOINT_IN : LIBUSB_ENDPOINT_OUT);

	ret = libusb_control_transfer(
			afm,		/* Device handle */
			type,		/* Request type */
			cmd,		/* bRequest*/
			0,			/* wValue */
			1,			/* wIndex (interface number) */
			data,
			len,
			USB_EP0_TIMEOUT);

	if (ret < 0) {
		Disconnect();
		return 0;
	}

	if (ret < 0) return 0;

	return 1;
}

int Device::Connect()
{
	/* Disconnect AFM if connected */
	if (afm) Disconnect();

	/* Try to open AFM device */
	afm = libusb_open_device_with_vid_pid(NULL, AFM_VID, AFM_PID);
	if (!afm) {
		return 0;
	}

	libusb_claim_interface(afm, 0);

	return 1;
}

void Device::Disconnect()
{
	if (afm) {
		libusb_release_interface(afm, 0);
		libusb_close(afm);
	}
	afm = NULL;
}

bool Device::IsConnected()
{
	return (afm != NULL);
}

int Device::GetFirmwareVersion()
{
	uint8_t buf[16];

	if (!afm) return 0;

	buf[0] = 0;
	buf[1] = 0;

	AfmCommand(AFM_GET_FIRMWARE_VERSION, DEVICE_GET, buf, 2);

	return (buf[0] << 8) + buf[1];
}

int Device::UpdateStatus()
{
	afm_t cmd;
	int ret;

	ret = AfmCommand(AFM_GET_STATUS, DEVICE_GET, (uint8_t *)&cmd, sizeof(cmd.afmGetStatus));
	if (!ret) return 0;

	return 1;
}

int Device::Run(int startX, int startY, uint16_t realsize, uint16_t pixelsize)
{
	afm_t cmd;

	cmd.afmRun.startX = startX;
	cmd.afmRun.startY = startY;
	cmd.afmRun.size = realsize;
	cmd.afmRun.res = pixelsize;
	return AfmCommand(AFM_RUN, DEVICE_SET, (uint8_t *)&cmd, sizeof(cmd.afmRun));
}

int Device::ReadData(uint8_t *buf, int len)
{
	int ret;
	int readlen;

	if (!afm) return 0;

	readlen = 0;
	ret = libusb_bulk_transfer(afm,
			AFM_BULK_IN,
			buf,
			len,
			&readlen,
			USB_BULK_TIMEOUT);

	return readlen;
}

int Device::ProcessDataPackets()
{
	uint8_t buf[64];
	uint8_t data[256];
	int ret, i;
	uint8_t *p;
	uint8_t cmd, datalen, datafollow;

	cmd = 0;
	datafollow = 0;
	while ( (ret = ReadData(buf, sizeof(buf))) > 0) {
		for (p = buf; p < (buf+ret); p++) {
			if (datafollow--) {
				/* Fetch data */
				data[datalen++] = *p;
				if (!datafollow) {
					ProcessDataPacket(cmd, datalen, data);
					cmd = 0;
				}
			} else {
				if (cmd) {
					/* Fetch data length */
					datafollow = *p;
					datalen = 0;
					if (!datafollow) {
						ProcessDataPacket(cmd, datalen, data);
						cmd = 0;
					}
				} else {
					/* Fetch command code */
					cmd = *p;
				}
			}
		}
		if (ret < sizeof(buf)) break;
	}

	if (ret < 0) return 0;
	return 1;
}

int Device::ProcessDataPacket(uint8_t cmd, uint8_t len, uint8_t *data)
{
	return 0;
}

int Device::ReadImage(AFMImage *image, void(*progress)(int percent))
{
	int ret;

	m_image = image;
	ret = ProcessDataPackets();
	m_image = NULL;

	return ret;
}
