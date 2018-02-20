/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Piotr Esden-Tempski <piotr@esden.net>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "button_boot.h"        // module's own include is always first.

#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/scb.h>

#include "gpio.h"

#define BLDR_ADDRESS	0x1FFF0000

static const gpio_pin button_pin = {
	.gp_port = GPIOC,
	.gp_pin  = GPIO1,
	.gp_mode = GPIO_MODE_INPUT,
};

static uint32_t button_debounce = 0;

void button_boot(void) {
#if 0
	/* Enable GPIOC clock. */
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);
	/* Set GPIO1 (in GPIO port C) to 'input open-drain'. */
	gpio_mode_setup(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO1);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);
#else
        gpio_init_pin(&button_pin);
#endif

  /* Check if the user button is depressed, if so launch the factory bootloader */
#if 0
	if ((GPIOC_IDR & (1 << 1)) == 0) {
#else
	if (button_pressed()) {
#endif
		/* Set vector table base address. */
		SCB_VTOR = BLDR_ADDRESS & 0xFFFF;
		/* Initialise master stack pointer. */
		asm volatile("msr msp, %0"::"g"
				 (*(volatile uint32_t *)BLDR_ADDRESS));
		/* Jump to bootloader. */
		(*(void (**)())(BLDR_ADDRESS + 4))();
	}
}

bool button_pressed(void) {
	return gpio_get(button_pin.gp_port, button_pin.gp_pin) == 0;
}

bool button_pressed_debounce(void) {

	if (button_pressed()) {
		button_debounce <<= 1;
		button_debounce |= 1;
	} else {
		button_debounce = 0;
	}

	return (button_debounce == 0xFFFFFFFF);
}

bool button_released_debounce(void) {

	if (!button_pressed()) {
		button_debounce <<= 1;
	} else {
		button_debounce = 0xFFFFFFFF;
	}

	return (button_debounce == 0);
}
