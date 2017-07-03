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

#ifndef LCD_included
#define LCD_included

#include <stdbool.h>

#include "gfx-pixtile.h"

#define LCD_WIDTH              240
#define LCD_HEIGHT             320
#define LCD_MAX_TILE_BYTES   65536
#define LCD_MAX_TILE_PIXELS  (LCD_MAX_TILE_BYTES / sizeof (gfx_rgb565))
#define LCD_MAX_TILE_ROWS    (LCD_MAX_TILE_PIXELS / LCD_WIDTH)

// Init the clocks, GPIO pins, timer, DMA controller, ILI9341 chip,
// and pixtile DMA buffers.
extern void lcd_init(void);

// Use alloc_pixtile to get DMA-capable tiles.
// Maximum size is 64 KB (32 Kpixels).
// Tiles are pre-cleared to the background color.
gfx_pixtile *lcd_alloc_pixtile(int x, int y, size_t w, size_t h);

// Send pixels to screen and deallocate tile.
void lcd_send_pixtile(gfx_pixtile *);

// Set the background pixel color.
//
// If immediate, the next tile allocated will have the new color.
// Otherwise, color change will take effect after 0-2 allocations.
extern void lcd_set_bg_color(gfx_rgb565 color, bool immediate);

// Get the background pixel color.
extern gfx_rgb565 lcd_bg_color(void);


#endif /* !LCD_included */
