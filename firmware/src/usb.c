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

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "usb_bsp.h"
#include "usbd_conf.h"
#include "usb.h"
#include "usb_dcd_int.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_usr.h"
#include "usb_class.h"

#include "protocol.h"
#include "microscope.h"


#define USB_TX_QUEUE_LENGTH		16


void OTG_FS_IRQHandler(void);
__ALIGN_BEGIN USB_OTG_CORE_HANDLE USB_OTG_dev __ALIGN_END;

static xQueueHandle *usbTxQueue;


void usbTask(void *p)
{
	/* Create queue for device->host bulk transfer */
	usbTxQueue = xQueueCreate(5, USB_TX_QUEUE_LENGTH);

	/* Initialize USB stack */
	USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);

	for (;;) {
		vTaskDelay(100);
	}
}

/***** STM32 USBD board support functions *****/

void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
{
	/* Enable USB pins */
	GPIOA->MODER |= (1 << 23) | (1 << 25);
	GPIOA->AFR[1] |= 0x000AA000;

	/* Enable USB clock */
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
}

void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
	/* Enable OTG FS interrupt */
	NVIC->ISER[2] |= (1 << (OTG_FS_IRQn % 32));
}

void USB_OTG_BSP_uDelay (const uint32_t usec)
{
  uint32_t count = 0;
  const uint32_t utime = (120 * usec / 7);
  do
  {
    if ( ++count > utime )
    {
      return ;
    }
  }
  while (1);
}

void USB_OTG_BSP_mDelay (const uint32_t msec)
{
	vTaskDelay(msec);
}

void OTG_FS_IRQHandler(void)
{
	USBD_OTG_ISR_Handler(&USB_OTG_dev);
}

/***** USB class callback functions *****/

static uint16_t AFM_Init(void);
static uint16_t AFM_DeInit(void);
static uint16_t AFM_Ctrl(uint32_t Cmd, uint8_t* Buf, uint32_t Len);
static uint16_t AFM_DataTx(uint8_t* Buf, uint16_t Len);
static uint16_t AFM_DataRx(uint8_t* Buf, uint32_t Len);

CDC_IF_Prop_TypeDef AFM_fops =
{
	AFM_Init,
	AFM_DeInit,
	AFM_Ctrl,
	AFM_DataTx,
	AFM_DataRx
};


static uint16_t AFM_Init(void)
{
	return USBD_OK;
}

static uint16_t AFM_DeInit(void)
{
	return USBD_OK;
}

static uint16_t AFM_Ctrl (uint32_t Cmd, uint8_t* Buf, uint32_t Len)
{
	uint8_t testBuf[64];
	afm_t *pkt = (afm_t *)Buf;
	config_t *cfg = micGetConfig();

	switch (Cmd) {
	case AFM_GET_FIRMWARE_VERSION:
		pkt->afmGetFirmwareVersion.fwMajorVersion = FWVERMAJOR;
		pkt->afmGetFirmwareVersion.fwMinorVersion = FWVERMINOR;
		break;

	case AFM_GET_STATUS:
		pkt->afmGetStatus.type = cfg->micType;
		pkt->afmGetStatus.status = AFM_STATUS_IDLE;
		pkt->afmGetStatus.zcontrol = AFM_ZCONTROL_OFF;
		pkt->afmGetStatus.height = 0;
		break;

	case AFM_RUN:
		testBuf[0] = AFM_IMAGE_START;
		testBuf[0] = 0x00;
		testBuf[0] = AFM_IMAGE_END;
		testBuf[0] = 0x00;
		xQueueSendFromISR(usbTxQueue, testBuf, NULL);
		break;
	}

	return USBD_OK;
}

/* Get next chunk of TX data
 *
 */
static uint16_t AFM_DataTx(uint8_t* Buf, uint16_t Len)
{
	if (xQueueReceiveFromISR(usbTxQueue, Buf, NULL) == pdTRUE) {
		return USB_TX_QUEUE_LENGTH;
	}

	return 0;
}

/* Silently discard Rx data */
static uint16_t AFM_DataRx(uint8_t* Buf, uint32_t Len)
{
	return USBD_OK;
}

/***** Generic USBD callbacks *****/

USBD_Usr_cb_TypeDef USR_cb =
{
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,


  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,
};

void USBD_USR_Init(void)
{
}

void USBD_USR_DeviceReset(uint8_t speed )
{
}


void USBD_USR_DeviceConfigured (void)
{
}

void USBD_USR_DeviceSuspended(void)
{
}


void USBD_USR_DeviceResumed(void)
{
}


void USBD_USR_DeviceConnected (void)
{
}


void USBD_USR_DeviceDisconnected (void)
{
}
