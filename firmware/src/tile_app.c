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

#include "tile_app.h"

#include "lcd.h"
#include "math-util.h"

#include "assets/assets.h"

#define TILE_SLICE_MAX_H (uint32_t)80 /* max height of a pixslice to procees in the renderer */

gfx_rgb565 color = 0;

static int16_t tile_y_inc = +1;
static int16_t tile_y = 0;

void tile_init(void)
{

}

void tile_animate(void)
{
	tile_y += tile_y_inc;
	if (tile_y + (TS_PIXMAP_HEIGHT*2) > LCD_HEIGHT) {
		tile_y_inc = -1;
		tile_y -= 2;
	} else if (tile_y < 0) {
		tile_y_inc = +1;
		tile_y += 1;
	}
}

static void tile_render_slice(gfx_pixslice *slice)
{

#if 0
	/* Draw some lines to indicate drawing direction and pixslice start/end */
	gfx_rgb565 *px =
		gfx_pixel_address_unchecked(slice, 0, slice->y);
	for (size_t x = 0; x < 10; x++) {
		*px++ = 0xFFFF;
	}
	px =
		gfx_pixel_address_unchecked(slice, 0, slice->y + slice->h - 2);
	for (size_t x = 0; x < 10; x++) {
		*px++ = 0xF800;
	}
#endif

	for (size_t y = slice->y; y < (slice->y + slice->h); y++ ) {
            	int32_t yy = (int32_t)y - tile_y;
                if (yy < 0 || yy >= (TS_PIXMAP_HEIGHT*2))
                    continue;
		gfx_rgb565 *px =
			gfx_pixel_address_unchecked(slice, 0, y);
		for (size_t x = 0; x < slice->w; x++) {
			if (x < (TS_PIXMAP_WIDTH*2)) {
				if(ts_pixmap[yy/2][x/2]!=0xF81F) {
					*px++ = ts_pixmap[yy/2][x/2];
				} else {
					px++;
				}
			}
		}
	}

}

void tile_render(void)
{
	size_t h;

	color = 0;

    for (size_t y = 0; y < LCD_HEIGHT; y += h) {
        h = MIN(TILE_SLICE_MAX_H, LCD_HEIGHT - y);
        gfx_pixslice *slice = lcd_alloc_pixslice(0, y, LCD_WIDTH, h);
        tile_render_slice(slice);
        lcd_send_pixslice(slice);
    }
}
