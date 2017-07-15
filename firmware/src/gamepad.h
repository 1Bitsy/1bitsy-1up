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

/* Snes Controller buttons */
#define GAMEPAD_BB		(1 << 15)
#define GAMEPAD_BY		(1 << 14)
#define GAMEPAD_BSELECT	(1 << 13)
#define GAMEPAD_BSTART	(1 << 12)
#define GAMEPAD_BUP		(1 << 11)
#define GAMEPAD_BDOWN	(1 << 10)
#define GAMEPAD_BLEFT	(1 << 9)
#define GAMEPAD_BRIGHT	(1 << 8)
#define GAMEPAD_BA		(1 << 7)
#define GAMEPAD_BX		(1 << 6)
#define GAMEPAD_BL		(1 << 5)
#define GAMEPAD_BR		(1 << 4)
#define GAMEPAD_BRES1	(1 << 3)
#define GAMEPAD_BRES2	(1 << 2)
#define GAMEPAD_BVOLM	(1 << 1)
#define GAMEPAD_BVOLP	(1 << 0)

void gamepad_init(void);
bool gamepad_present(void);
uint16_t gamepad_get(void);

#endif /* GAMEPAD_H */