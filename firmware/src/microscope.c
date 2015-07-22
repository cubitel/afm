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

#include <stm32f4xx.h>

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
 * PB3	CTL_CLK			SPI clock for VGA/VCM (SPI1)
 * PB5	CTL_SDI			SPI data for VGA/VCM (SPI1)
 * PB6	CTL_VGA			Variable gain amplifier chip select
 * PB7	ADC_SELECT		ADC_IN source select
 * PB8	CTL_VCM			Voice coil motor DAC chip select
 * PB9	VCM_MUTE		Voice coil motor MUTE signal
 * PB10	OPU_ENA			OPU laser enable signal
 * PB14	PZ_LDAC			Piezo DAC LDAC signal
 *
 * PC10	PZ_CLK			Piezo DAC SPI clock (I2S3)
 * PC11	PZ_CS			Piezo DAC chip select (I2S3_ext)
 * PC12	PZ_SDI			Piezo DAC SPI data (I2S3)
 *
 * PD12	STATUS_GR		Status green LED
 * PD13	STATUS_RD		Status red LED
 * PD14	PROBE_GR		Probe green LED
 * PD15	PROBE_RD		Probe red LED
 */

#define LED_STATUS_GRN	(1 << 12)
#define LED_STATUS_RED	(1 << 13)
#define LED_PROBE_GRN	(1 << 14)
#define LED_PROBE_RED	(1 << 15)

/* I2S PLL clock = PLLM (1 MHz) * N / R */
#define I2S_PLL_N		200		/* 192..432 */
#define I2S_PLL_R		2		/* 2..7 */
/* I2S clock = I2S PLL clock / (DIV * 2) */
#define I2S_DIV			16		/* 2..255 */

/* DAC channels */
#define DAC_X			0
#define DAC_Y			1
#define DAC_Z			2
#define DAC_BIAS		3

/* Piezo DAC DMA circular buffer
 * 3 words (X, Y, Z) and 16->20 bit DAC extension (x16)
 * give us 48 words buffer size
 */
#define DAC_VALUE_COUNT	16
#define DAC_BUFFER_SIZE	(DAC_VALUE_COUNT * 3)

/* High 16 bits are DAC value, low 2 bits are DAC channel */
static uint32_t dacDMABuffer[DAC_BUFFER_SIZE];
static uint32_t dacChipSelect = 0x0000FFFC;


/* Microscope configuration */
static config_t cfg;

/* DAC setpoints */
static uint32_t xSet;
static uint32_t ySet;
static uint32_t zSet;
static uint32_t biasSet;

/* ADC setpoint */
static uint32_t adcSet;


void DMA1_Stream7_IRQHandler(void);


static void gpioSetMode(GPIO_TypeDef *gpio, uint8_t pin, uint8_t mode)
{
	uint32_t mask;

	mask = 3 << (pin << 1);
	gpio->MODER = (gpio->MODER & ~mask) | (mode << (pin << 1));
}

static void gpioSetSpeed(GPIO_TypeDef *gpio, uint8_t pin, uint8_t speed)
{
	uint32_t mask;

	mask = 3 << (pin << 1);
	gpio->OSPEEDR = (gpio->OSPEEDR & ~mask) | (speed << (pin << 1));
}

static void gpioSetAltFunc(GPIO_TypeDef *gpio, uint8_t pin, uint8_t fn)
{
	uint32_t mask;

	gpioSetMode(gpio, pin, 2);

	if (pin < 8) {
		mask = 15 << (pin << 2);
		gpio->AFR[0] = (gpio->AFR[0] & ~mask) | (fn << (pin << 2));
	} else {
		pin &= 7;
		mask = 15 << (pin << 2);
		gpio->AFR[1] = (gpio->AFR[1] & ~mask) | (fn << (pin << 2));
	}
}

static void ledSet(uint32_t leds)
{
	GPIOD->ODR = (GPIOD->ODR & ~(LED_STATUS_GRN | LED_STATUS_RED | LED_PROBE_GRN | LED_PROBE_RED)) | leds;
}

static void fillDACBuffer(uint32_t *buf)
{
	int i;
	uint32_t x, y, z;
	uint32_t zdev;
	uint32_t adc;

	switch (cfg.zcontrol) {
	case AFM_ZCONTROL_OFF:
		xSet = 0x80000000;
		ySet = 0x80000000;
		zSet = 0x80000000;
		break;
	case AFM_ZCONTROL_ON:
		if (ADC1->SR & ADC_SR_EOC) {
			adc = ADC1->DR;
			zdev = 0x00001000;
			/* Probe is lower */
			if (adc > adcSet) {
				if (zSet < (0xFFFFFFFF - zdev)) zSet += zdev;
			}
			/* Probe is higher */
			if (adc < adcSet) {
				if (zSet > zdev) zSet -= zdev;
			}
		}
		/* Start new conversation on ADC1 */
		ADC1->CR2 |= ADC_CR2_SWSTART;
		break;
	}

	x = xSet;
	y = ySet;
	z = zSet;

	/* Fill buffer with sigma-delta modulated values */
	for (i = 0; i < DAC_VALUE_COUNT; i++) {
		x = (x & 0xFFFF) + xSet;
		*buf++ = (x & 0xFFFF0000) | DAC_X;
		y = (y & 0xFFFF) + ySet;
		*buf++ = (y & 0xFFFF0000) | DAC_Y;
		z = (z & 0xFFFF) + zSet;
		*buf++ = (z & 0xFFFF0000) | DAC_Z;
	}
}

static void dacStart(void)
{
	fillDACBuffer(dacDMABuffer);

	/* Start SPI3 */
	DMA1_Stream7->CR |= DMA_SxCR_EN;
	DMA1_Stream5->CR |= DMA_SxCR_EN;
	I2S3ext->I2SCFGR |= SPI_I2SCFGR_I2SE;
	SPI3->I2SCFGR |= SPI_I2SCFGR_I2SE;
}

void micInit()
{
	/* Default configuration */
	cfg.micType = AFM_TYPE_STM;
	cfg.zcontrol = AFM_ZCONTROL_OFF;
	cfg.afm.amplitude = 75;
	cfg.afm.freq = 0;
	cfg.stm.bias = 50;
	cfg.stm.current = 100;

	/* Zero DAC outputs */
	xSet = 0x80000000;
	ySet = 0x80000000;
	zSet = 0x80000000;
	biasSet = 0x80000000;
	adcSet = 1000;

	/***** Configure hardware *****/

	/* Enable peripheral clocks */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_DMA1EN;

	/* PA2 -- ADC IN 2, Analog */
	gpioSetMode(GPIOA, 2, 3);
	/* PA15 -- I2S3_WS (Not connected, but needed for I2S3ext) */
	gpioSetAltFunc(GPIOA, 15, 6);
	gpioSetSpeed(GPIOA, 15, 2);

	/* PC10 -- I2S3_CK, Fast */
	gpioSetAltFunc(GPIOC, 10, 6);
	gpioSetSpeed(GPIOC, 10, 2);
	/* PC11 -- I2S3ext_SD, Fast */
	gpioSetAltFunc(GPIOC, 11, 5);
	gpioSetSpeed(GPIOC, 11, 2);
	/* PC12 -- I2S3_SD, Fast */
	gpioSetAltFunc(GPIOC, 12, 6);
	gpioSetSpeed(GPIOC, 12, 2);

	/* PD12..PD15 -- Outputs */
	gpioSetMode(GPIOD, 12, 1);
	gpioSetMode(GPIOD, 13, 1);
	gpioSetMode(GPIOD, 14, 1);
	gpioSetMode(GPIOD, 15, 1);

	/* Configure SPI3
	 *
	 * "Thanks" to ST which ignores CKPOL bit in PCM mode and makes it unusable here.
	 * So, we use SPI3 in I2S full-duplex with both channels in transmit mode.
	 * I2S3_SD used for data line, and I2S3ext_SD used for chip select line.
	 */

	/* Enable SPI3 */
	RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
	/* I2S clock source -- PLL */
	RCC->CFGR &= ~RCC_CFGR_I2SSRC;
	/* Set PLL frequency */
	RCC->PLLI2SCFGR = (I2S_PLL_R << 28) | (I2S_PLL_N << 6);
	/* Enable I2S PLL */
	RCC->CR |= RCC_CR_PLLI2SON;
	/* Wait for PLL ready */
	while (!(RCC->CR & RCC_CR_PLLI2SRDY)) {}
	/* I2S prescaler */
	SPI3->I2SPR = I2S_DIV;
	/* I2S mode, master, transmit, 32 bit */
	SPI3->I2SCFGR = SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_I2SCFG_1 | SPI_I2SCFGR_I2SSTD_0 | SPI_I2SCFGR_DATLEN_1;
	/* I2Sext, slave, transmit, 32 bit */
	I2S3ext->I2SCFGR = SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_I2SSTD_0 | SPI_I2SCFGR_DATLEN_1;

	/* DMA setup: Channel 0, Memory to peripheral, 16 bit, circular mode, transfer complete interrupt */
	DMA1_Stream7->CR = DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0 | DMA_SxCR_MINC | DMA_SxCR_CIRC | DMA_SxCR_DIR_0 | DMA_SxCR_TCIE;
	/* DMA buffer start address */
	DMA1_Stream7->M0AR = (uint32_t)dacDMABuffer;
	/* Destination -- SPI3 DR */
	DMA1_Stream7->PAR = (uint32_t)&SPI3->DR;
	/* Data items count */
	DMA1_Stream7->NDTR = DAC_BUFFER_SIZE * 2;
	/* Disable direct mode */
	DMA1_Stream7->FCR = DMA_SxFCR_DMDIS;

	/* DMA setup: Channel 2, Memory to peripheral, 16 bit, circular mode */
	DMA1_Stream5->CR = (2 << 25) | DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0 | DMA_SxCR_MINC | DMA_SxCR_CIRC | DMA_SxCR_DIR_0;
	/* DMA buffer start address */
	DMA1_Stream5->M0AR = (uint32_t)&dacChipSelect;
	/* Destination -- I2S3ext DR */
	DMA1_Stream5->PAR = (uint32_t)&I2S3ext->DR;
	/* Data items count */
	DMA1_Stream5->NDTR = 2;
	/* Disable direct mode */
	DMA1_Stream5->FCR = DMA_SxFCR_DMDIS;

	/* Enable DMA IRQ */
	NVIC->ISER[1] |= (1 << (DMA1_Stream7_IRQn & 0x1F));
	/* Enable I2S DMA */
	SPI3->CR2 = SPI_CR2_TXDMAEN;
	I2S3ext->CR2 = SPI_CR2_TXDMAEN;

	/* Configure ADC */

	/* Enable ADC1 */
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	/* 12 bit, scan mode */
	ADC1->CR1 = ADC_CR1_SCAN;
	/* 1 input channel */
	ADC1->SQR1 = 0;
	/* Select channel 2 */
	ADC1->SQR3 = 2;
	/* Enable ADC */
	ADC1->CR2 = ADC_CR2_ADON;


	/* Start piezo DAC */
	dacStart();

	ledSet(LED_STATUS_GRN);
}

config_t *micGetConfig(void)
{
	return &cfg;
}

void DMA1_Stream7_IRQHandler()
{
	/* Clear interrupt flag */
	DMA1->HIFCR = DMA_HIFCR_CTCIF7;
}
