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

#include <libopencm3/stm32/gpio.h>

#include "munch_app.h"

#include "lcd.h"
#include "math-util.h"

/* The classic Munching Square eye candy app. */

gfx_rgb565 munch_base_color;

// uint16_t munch_magic = 27;         // try different values
uint16_t munch_magic = 20 << 11 | 27; // This is more colorful.


void munch_init(void)
{

}

void munch_animate(void)
{
    /* We want to change the magic number on every color overflow. */
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wstrict-overflow"
//     if (munch_base_color > (munch_base_color + 0x0021)) {
// #pragma GCC diagnostic pop
//         munch_magic+=10;
//         if (munch_magic > 100) {
//             munch_magic = 10;
//         }
//     }
    munch_base_color += 0x0021;
}

/* Borrow the framerate function from tile app. */
extern void tile_draw_fps(gfx_pixslice *slice);

static void munch_render_slice(gfx_pixslice *slice)
{
    const int y_off = -8;
    const int x_off = 32;

    gpio_set(GPIOA, GPIO3);

    int y0 = MAX(0, slice->y - y_off);
    int y1 = MIN(256, slice->y + (int)slice->h - y_off);
    int x0 = MAX(0, slice->x - x_off);
    int x1 = MIN(256, x0 + slice->w - x_off);
    gfx_rgb565 base = munch_base_color;
    for (int y = y0; y < y1; y++) {
        gfx_rgb565 *p =
            gfx_pixel_address_unchecked(slice, x0 + x_off, y + y_off);
        for (int x = x0; x < x1; x++)
            *p++ = base + munch_magic * (x ^ y);
    }

    if (slice->y == 0) {
        tile_draw_fps(slice);
    }

    gpio_clear(GPIOA, GPIO3);
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
