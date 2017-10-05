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

#ifndef LCD_H
#define LCD_H

#include <stdbool.h>

#include "gfx-pixslice.h"

#define LCD_WIDTH              320
#define LCD_HEIGHT             240
#define LCD_MAX_SLICE_ROWS    (LCD_HEIGHT / 5)
#define LCD_MAX_SLICE_PIXELS  (LCD_MAX_SLICE_ROWS * LCD_WIDTH)
#define LCD_MAX_SLICE_BYTES   (LCD_MAX_SLICE_PIXELS * sizeof (gfx_rgb565))

// Init the clocks, GPIO pins, timer, DMA controller, ILI9341 chip,
// and pixslice DMA buffers.
void lcd_init(void);

// Use alloc_pixslice to get DMA-capable slices.
// Maximum size is 64 KB (32 Kpixels).
// Slices are pre-cleared to the background color.
gfx_pixslice *lcd_alloc_pixslice(int x, int y, size_t w, size_t h);

// Send pixels to screen and deallocate slice.
void lcd_send_pixslice(gfx_pixslice *);

// Set the background pixel color.
//
// If immediate, the next slice allocated will have the new color.
// Otherwise, color change will take effect after 0-2 allocations.
void lcd_set_bg_color(gfx_rgb565 color, bool immediate);

// Get the background pixel color.
gfx_rgb565 lcd_bg_color(void);

// Backlight control
void lcd_bl_on(void);
void lcd_bl_off(void);
void lcd_bl_toggle(void);

#endif /* !LCD_H */
