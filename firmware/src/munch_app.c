/*
 * This file is part of the 1Bitsy 1UP retro inspired game console project.
 *
 * Copyright (C) 2017 Bob Miller <kbob@jogger-egg.com>
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

#include "munch_app.h"

#include "lcd.h"
#include "math-util.h"

/* The classic Munching Square eye candy app. */

gfx_rgb565 base_color;

#define MAGIC 27                // try different values

void munch_init(void)
{

}

void munch_animate(void)
{
	base_color += 0x0021;
}

static void munch_render_slice(gfx_pixslice *slice)
{
    const int y_off = 32;
    const int x_off = -8;

    int y0 = MAX(0, slice->y - y_off);
    int y1 = MIN(256, slice->y + (int)slice->h - y_off);
    int x0 = slice->x - x_off;
    int x1 = x0 + slice->w - x_off;
    gfx_rgb565 base = base_color;
    for (int y = y0; y < y1; y++) {
        gfx_rgb565 *p =
            gfx_pixel_address_unchecked(slice, x0 + x_off, y + y_off);
        for (int x = x0; x < x1; x++)
            *p++ = base + MAGIC * (x ^ y);
    }
}

void munch_render(void)
{
	size_t h;

    for (size_t y = 0; y < LCD_HEIGHT; y += h) {
        h = MIN(LCD_MAX_SLICE_ROWS, LCD_HEIGHT - y);
        gfx_pixslice *slice = lcd_alloc_pixslice(0, y, LCD_WIDTH, h);
        munch_render_slice(slice);
        lcd_send_pixslice(slice);
    }
}