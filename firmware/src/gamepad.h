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

#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <stdbool.h>
#include <stdint.h>

/* SNES Controller buttons */
#define GAMEPAD_BB_BIT	    15
#define GAMEPAD_BY_BIT	    14
#define GAMEPAD_BSELECT_BIT 13
#define GAMEPAD_BSTART_BIT  12
#define GAMEPAD_BUP_BIT	    11
#define GAMEPAD_BDOWN_BIT   10
#define GAMEPAD_BLEFT_BIT   9
#define GAMEPAD_BRIGHT_BIT  8
#define GAMEPAD_BA_BIT	    7
#define GAMEPAD_BX_BIT	    6
#define GAMEPAD_BL_BIT	    5
#define GAMEPAD_BR_BIT	    4
#define GAMEPAD_BRES1_BIT   3
#define GAMEPAD_BRES2_BIT   2
#define GAMEPAD_BVOLM_BIT   1
#define GAMEPAD_BVOLP_BIT   0

#define GAMEPAD_BB      (1 << GAMEPAD_BB_BIT)
#define GAMEPAD_BY      (1 << GAMEPAD_BY_BIT)
#define GAMEPAD_BSELECT (1 << GAMEPAD_BSELECT_BIT)
#define GAMEPAD_BSTART  (1 << GAMEPAD_BSTART_BIT)
#define GAMEPAD_BUP     (1 << GAMEPAD_BUP_BIT)
#define GAMEPAD_BDOWN   (1 << GAMEPAD_BDOWN_BIT)
#define GAMEPAD_BLEFT   (1 << GAMEPAD_BLEFT_BIT)
#define GAMEPAD_BRIGHT  (1 << GAMEPAD_BRIGHT_BIT)
#define GAMEPAD_BA      (1 << GAMEPAD_BA_BIT)
#define GAMEPAD_BX      (1 << GAMEPAD_BX_BIT)
#define GAMEPAD_BL      (1 << GAMEPAD_BL_BIT)
#define GAMEPAD_BR      (1 << GAMEPAD_BR_BIT)
#define GAMEPAD_BRES1   (1 << GAMEPAD_BRES1_BIT)
#define GAMEPAD_BRES2   (1 << GAMEPAD_BRES2_BIT)
#define GAMEPAD_BVOLM   (1 << GAMEPAD_BVOLM_BIT)
#define GAMEPAD_BVOLP   (1 << GAMEPAD_BVOLP_BIT)

void gamepad_init(void);
bool gamepad_present(void);
uint16_t gamepad_get(void);

#endif /* GAMEPAD_H */
