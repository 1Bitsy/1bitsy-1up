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

#ifndef GPIO_H
#define GPIO_H

#include <stddef.h>

#include <libopencm3/stm32/gpio.h>

typedef struct gpio_pin {
    uint32_t gp_port;           // GPIOA .. GPIOF
    uint16_t gp_pin;            // GPIO0 .. GPIO15 (may OR pins)
    uint8_t  gp_mode   : 2;     // GPIO_MODE_INPUT/OUTPUT/AF/ANALOG
    uint8_t  gp_pupd   : 2;     // GPIO_PUPD_NONE/PULLUP/PULLDOWN
    uint8_t  gp_af     : 4;     // GPIO_AF0 .. GPIO_AF15
    uint8_t  gp_ospeed : 2;     // GPIO_OSPEED_2/25/60/100MHZ
    uint8_t  gp_otype  : 1;     // GPIO_OTYPE_PP/OD (push-pull, open drain)
    uint8_t  gp_level  : 1;     // 0 or 1
} gpio_pin;

// Initialization asserts that each pin is not yet used and
// enables RCC peripheral clocks as needed before configuring
// the pin(s).
//
// Configuration just sets the mode and other attributes.
// Does not init the clock or check for uniqueness.

void gpio_init_pin(const gpio_pin *);
void gpio_config_pin(const gpio_pin *);


// Convenience functions to init/config several pins at once.
void gpio_init_pins(const gpio_pin *, size_t count);
void gpio_config_pins(const gpio_pin *, size_t count);

#endif /* GPIO_H */
