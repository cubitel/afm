#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__

#include "usb_conf.h"

#define USBD_CFG_MAX_NUM                1
#define USBD_ITF_MAX_NUM                1
#define USB_MAX_STR_DESC_SIZ            50 

#define CDC_IN_EP                       0x81  /* EP1 for data IN */
#define CDC_OUT_EP                      0x01  /* EP1 for data OUT */
#define CDC_CMD_EP                      0x82  /* EP2 for CDC commands */

/* CDC Endpoints parameters: you can fine tune these values depending on the needed baudrates and performance. */
#define CDC_DATA_MAX_PACKET_SIZE       64   /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SZE             16   /* Control Endpoint Packet size */

#define CDC_IN_FRAME_INTERVAL          1    /* Number of micro-frames between IN transfers */
#define USB_TX_BUFF_SIZE               256  /* Total size of IN buffer */
#define USB_RX_BUFF_SIZE               256  /* Total size of OUT buffer */

#define APP_FOPS                        AFM_fops

#endif //__USBD_CONF__H__
