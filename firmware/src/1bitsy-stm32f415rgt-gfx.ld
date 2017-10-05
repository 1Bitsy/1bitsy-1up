/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
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

/* Linker script for 1Bitsy (STM32F415RGT, 1024K flash, 128K RAM). */

/* Define memory regions. */
MEMORY
{
	rom   (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
	ram   (rwx) : ORIGIN = 0x10000000, LENGTH =   64K /* Core Coupled RAM */
        sram1 (rwx) : ORIGIN = 0x20000000, LENGTH =  112K /* System RAM 1 */
        sram2 (rwx) : ORIGIN = 0x2001C000, LENGTH =   16K /* System RAM 2 */
}

SECTIONS
{
	.sram1 (NOLOAD) : {
		sram1_start = .;
		sram1_end = . + 112K;
		*(.sram1)
	} >sram1
	.sram2 (NOLOAD) : {
		sram2_start = .;
		sram2_end = . + 16K;
		*(.sram2)
	} >sram2
}

/* Include the common ld script. */
INCLUDE libopencm3_stm32f4.ld

