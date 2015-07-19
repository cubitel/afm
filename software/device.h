#ifndef DEVICE_H_
#define DEVICE_H_

#include <libusb.h>

#include "image.h"

#define DEVICE_GET		0
#define DEVICE_SET		1

class Device {
private:
	libusb_device_handle *afm;
	AFMImage *m_image;
	int AfmCommand(uint8_t cmd, uint8_t direction, uint8_t *data, int len);
public:
	Device(void);
	int Connect();
	void Disconnect();
	bool IsConnected();
	int GetFirmwareVersion();
	int UpdateStatus();
	int Run(int startX, int startY, uint16_t realsize, uint16_t pixelsize);
	int ReadData(uint8_t *buf, int len);
	int ProcessDataPackets();
	int ProcessDataPacket(uint8_t cmd, uint8_t len, uint8_t *data);
	int ReadImage(AFMImage *image, void(*progress)(int percent));
};


#endif /* DEVICE_H_ */
