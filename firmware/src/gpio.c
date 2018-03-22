/*
 * This file is part of the 1Bitsy lcd graphics library.
 *
 * Copyright (C) 2017 Bob Miller <kbob@jogger-egg.com>
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

#include "gpio.h"

#include <assert.h>
#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>

#define GPIO_PORT_COUNT 11

static uint16_t gpio_pins_used[GPIO_PORT_COUNT];

void gpio_init_pin(const gpio_pin *pin)
{
    uint32_t port = pin->gp_port;
    uint16_t pinmask = pin->gp_pin;
    uint32_t index = ((uint32_t)port - (uint32_t)PERIPH_BASE_AHB1) >> 10;
    assert(index < GPIO_PORT_COUNT);

    if (!gpio_pins_used[index])
        rcc_periph_clock_enable((0x30 << 5) | index);

    assert(!(gpio_pins_used[index] & pinmask));
    gpio_pins_used[index] |= pinmask;

    gpio_config_pin(pin);
}

void gpio_config_pin(const gpio_pin *pin)
{
    uint32_t port = pin->gp_port;
    uint16_t pinmask = pin->gp_pin;

    gpio_mode_setup(port,
                    pin->gp_mode,
                    pin->gp_pupd,
                    pinmask);

    if (pin->gp_mode == GPIO_MODE_OUTPUT) {
        if (pin->gp_level)
            gpio_set(port, pinmask);
        else
            gpio_clear(port, pinmask);
    }

    if (pin->gp_mode == GPIO_MODE_OUTPUT || pin->gp_mode == GPIO_MODE_AF)
        gpio_set_output_options(port,
                                pin->gp_otype,
                                pin->gp_ospeed,
                                pinmask);

    if (pin->gp_mode == GPIO_MODE_AF)
        gpio_set_af(port,
                    pin->gp_af,
                    pinmask);
}

void gpio_init_pins(const gpio_pin *pins, size_t count)
{
    for (size_t i = 0; i < count; i++)
        gpio_init_pin(&pins[i]);
}

void gpio_config_pins(const gpio_pin *pins, size_t count)
{
    for (size_t i = 0; i < count; i++)
        gpio_config_pin(&pins[i]);
}
