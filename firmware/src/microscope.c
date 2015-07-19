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

#include "microscope.h"

/* Hardware connections
 *
 * PA1	ADC_FOCUS		OPU focusing
 * PA2	ADC_IN			ADC Input signal
 * PA4	DAC_ZMODP		Cantilever modulation signal +
 * PA5	DAC_ZMODN		Cantilever modulation signal -
 * PA11	USBDM
 * PA12	USBDP
 *
 * PB3	CTL_CLK			SPI clock for VGA/VCM (SPI3)
 * PB5	CTL_SDI			SPI data for VGA/VCM (SPI3)
 * PB6	CTL_VGA			Variable gain amplifier chip select
 * PB7	ADC_SELECT		ADC_IN source select
 * PB8	CTL_VCM			Voice coil motor DAC chip select
 * PB9	VCM_MUTE		Voice coil motor MUTE signal
 * PB10	OPU_ENA			OPU laser enable signal
 * PB12	PZ_CS			Piezo DAC chip select
 * PB13	PZ_CLK			Piezo DAC SPI clock (I2S 2)
 * PB14	PZ_LDAC			Piezo DAC LDAC signal
 * PB15	PZ_SDI			Piezo DAC SPI data (I2S 2)
 *
 * PD12	STATUS_GR		Status green LED
 * PD13	STATUS_RD		Status red LED
 * PD14	PROBE_GR		Probe green LED
 * PD15	PROBE_RD		Probe red LED
 */

static config_t cfg;

void micInit()
{
	/* Default configuration */
	cfg.micType = AFM_TYPE_STM;
	cfg.zcontrol = AFM_ZCONTROL_OFF;
	cfg.afm.amplitude = 75;
	cfg.afm.freq = 0;
	cfg.stm.bias = 50;
	cfg.stm.current = 100;
}

config_t *micGetConfig(void)
{
	return &cfg;
}
