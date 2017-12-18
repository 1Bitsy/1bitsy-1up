/*
 * This file is part of the 1Bitsy 1UP retro inspired game console project.
 *
 * Copyright (C) 2017 Nicolas Schodet <nico@ni.fr.eu.org>
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
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

#include "i2c.h"
#include "lcd.h"

#include "touch.h"

static const i2c_channel touch_i2c = {
	.i_base_address = I2C1,
	.i_is_master = true,
	.i_stop = true,
	.i_address = 0b01110000,
};

static volatile bool touch_event = true;

static bool touch_touched = false;
static int touch_x = 0;
static int touch_y = 0;

void exti9_5_isr(void)
{
	touch_event = true;
	exti_reset_request(EXTI5);
}

void touch_init(void)
{
	rcc_periph_clock_enable(RCC_SYSCFG);

	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO5);
	exti_select_source(EXTI5, GPIOB);
	exti_set_trigger(EXTI5, EXTI_TRIGGER_FALLING);
	exti_enable_request(EXTI5);
	exti_reset_request(EXTI5);
	nvic_enable_irq(NVIC_EXTI9_5_IRQ);
}

static void touch_update(void)
{
	static const uint8_t req[] = { 0x02 };
	i2c_transmit(&touch_i2c, req, sizeof(req));
	uint8_t rsp[5] = { 0 };
	i2c_receive(&touch_i2c, rsp, sizeof(rsp));
	int count = rsp[0] & 0xf;
	if (count && count <= 5) {
		touch_touched = true;
		touch_y = LCD_HEIGHT - ((rsp[1] & 0xf) << 8 | rsp[2]);
		touch_x = ((rsp[3] & 0xf) << 8 | rsp[4]);
	} else {
		touch_touched = false;
	}
}

bool touch_get(int *x, int *y)
{
	if (touch_event) {
		touch_event = false;
		touch_update ();
	}
	*x = touch_x;
	*y = touch_y;
	return touch_touched;
}
