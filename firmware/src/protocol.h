#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#ifndef __PACKED__
#define __PACKED__ __attribute__((packed))
#endif

/* Get current firmware version number */
#define AFM_GET_FIRMWARE_VERSION	0x00

struct afmGetFirmwareVersion {
	uint8_t			fwMajorVersion;
	uint8_t			fwMinorVersion;
} __PACKED__;


/* Get microscope status */
#define AFM_GET_STATUS				0x01

#define AFM_TYPE_NONE			0
#define AFM_TYPE_AFM			1
#define AFM_TYPE_STM			2

#define AFM_STATUS_IDLE			0
#define AFM_STATUS_RUNNING		1

#define AFM_ZCONTROL_OFF		0	/* All outputs is set to 0V */
#define AFM_ZCONTROL_ON			1	/* Constant altitude (altitude control depended on type) */
#define AFM_ZCONTROL_CONST		2	/* Constant height */

struct afmGetStatus {
	uint8_t			type;
	uint8_t			status;
	uint8_t			zcontrol;
	uint8_t			height;		/* Current Z position (0-100%) */
} __PACKED__;


/* Set microscope type */
#define AFM_SET_TYPE				0x02

struct afmSetType {
	uint8_t			type;
} __PACKED__;


/* Set Z control method */
#define AFM_SET_ZCONTROL			0x03

struct afmSetZControl {
	uint8_t			zcontrol;
} __PACKED__;


/* Get/Set AFM properties */
#define AFM_GET_AFM_PROP			0x04
#define AFM_SET_AFM_PROP			0x05

struct afmAFMProp {
	uint8_t			amplitude;		/* AFM: Cantilever amplitude setpoint, in percent */
	uint32_t		freq;			/* Cantilever frequency */
} __PACKED__;


/* Get/Set STM properties */
#define AFM_GET_STM_PROP			0x06
#define AFM_SET_STM_PROP			0x07

struct afmSTMProp {
	uint16_t		bias;			/* STM: Bias voltage, in mV */
	uint16_t		current;		/* STM: Current setpoint, in 0.01 nA */
} __PACKED__;


/* Start scanning */
#define AFM_RUN						0x08

struct afmRun {
	int32_t		startX;				/* Scan X start point, in nanometers */
	int32_t		startY;				/* Scan Y start point, in nanometers */
	uint16_t	size;				/* Scan size, in nanometers */
	uint16_t	res;				/* Scan size, in pixels */
} __PACKED__;


/* Stop scanning */
#define AFM_STOP					0x09


/* All packet types in one union */
typedef union {
	struct afmGetFirmwareVersion afmGetFirmwareVersion;
	struct afmGetStatus afmGetStatus;
	struct afmSetType afmSetType;
	struct afmSetZControl afmSetZControl;
	struct afmAFMProp afmAFMProp;
	struct afmSTMProp afmSTMProp;
	struct afmRun afmRun;
} afm_t;


/* Data messages transferred over EP1
 * Each message starts with one byte message code, followed by one byte data length.
 *
 */

#define AFM_IMAGE_START				0x80
#define AFM_IMAGE_DATA				0x81
#define AFM_IMAGE_END				0x82

#endif /* PROTOCOL_H_ */
