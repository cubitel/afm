#ifndef MICROSCOPE_H_
#define MICROSCOPE_H_


#include <FreeRTOS.h>
#include "protocol.h"


typedef struct {
	uint8_t				micType;
	uint8_t				zcontrol;
	struct afmAFMProp	afm;
	struct afmSTMProp	stm;
} config_t;

void micInit(void);
config_t *micGetConfig(void);


#endif /* MICROSCOPE_H_ */
