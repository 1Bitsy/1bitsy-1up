/*
 * This file is part of the 1Bitsy 1UP retro inspired game console project.
 *
 * Copyright (C) 2017 Piotr Esden-Tempski <piotr@esden.net>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/gpio.h>

#include "gamepad.h"

void gamepad_init(void)
{
	/* LATCH */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);
	gpio_clear(GPIOC, GPIO4);
	/* CLOCK */
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7);
	gpio_clear(GPIOA, GPIO4);
	/* DATA */
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO6);
}

bool gamepad_present(void)
{
	return (gamepad_get() != 0xFFFFF );
}

uint16_t gamepad_get(void)
{
	uint16_t ret = 0;

	/* assert latch */
	gpio_clear(GPIOC, GPIO4);
	for (int i = 0; i < 30; i++) asm("nop");
	gpio_set(GPIOC, GPIO4);
	for (int i = 0; i < 30; i++) asm("nop");

	/* read bit 1 */
	if (gpio_get(GPIOA, GPIO6) != 0) {
		ret <<= 1;
		ret |= 1;
	} else {
		ret <<= 1;
	}

	/* Read another 16 bit */
	for (int i = 0; i < 15; i++) {
		gpio_set(GPIOA, GPIO7);
		for (int d = 0; d < 30; d++) asm("nop");
		gpio_clear(GPIOA, GPIO7);
		for (int d = 0; d < 30; d++) asm("nop");
		if (gpio_get(GPIOA, GPIO6) != 0) {
			ret <<= 1;
			ret |= 1;
		} else {
			ret <<= 1;
		}
	}

	return (ret ^ 0xFFFFF);
}