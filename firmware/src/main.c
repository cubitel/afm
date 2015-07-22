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
#include <stm32f4xx.h>

#include "usb.h"
#include "microscope.h"


int main(void)
{
	/* Init microscope hardware */
	micInit();

	/* Create USB task */
	xTaskCreate(usbTask, "USB", 256, NULL, 1, NULL);

	/* Start FreeRTOS task scheduler */
	vTaskStartScheduler();

	/* We never reached here */
	while (1);
}
