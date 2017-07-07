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

//static int16_t tile_y_inc = +1;
//static int16_t tile_y = 0;

static int16_t map_x_off = 0;
static int16_t map_x_dir = +1;

static struct tile_video_state {
	uint16_t const *tilemap;
	uint16_t tilemap_w;
	uint16_t tilemap_h;
	uint16_t const *tilesheet;
	uint16_t tilesheet_w;
	uint16_t tilesheet_h;
	uint16_t x_off;
	uint16_t y_off;
} tile_video_state;

void tile_init(void)
{
	tile_video_state.tilemap = &tml1_tilemap[0][0][0];
	tile_video_state.tilemap_w = TML1_TILEMAP_WIDTH;
	tile_video_state.tilemap_h = TML1_TILEMAP_HEIGHT;

	tile_video_state.tilesheet = &ts_pixmap[0][0];
	tile_video_state.tilesheet_w = TS_PIXMAP_WIDTH;
	tile_video_state.tilesheet_h = TS_PIXMAP_HEIGHT;

	tile_video_state.x_off = 0;
	tile_video_state.y_off = 0;

}

void tile_draw_tile(gfx_pixslice *slice, uint16_t tile_id, int px, int py)
{
	int tile_x = (tile_id * 8) % TS_PIXMAP_WIDTH;
	int tile_y = ((tile_id * 8) / TS_PIXMAP_WIDTH) * 8;

	if (tile_x >= TS_PIXMAP_WIDTH)
	{
		return;
	}

	if (tile_y >= TS_PIXMAP_HEIGHT)
	{
		return;
	}

	for (int y = 0; y < 8; y++) {
		gfx_rgb565 *px0 =
			gfx_pixel_address_unchecked(slice, px, py + (y * 2));
		gfx_rgb565 *px1 =
			gfx_pixel_address_unchecked(slice, px, py + (y * 2) + 1);
		for (int x = 0; x < 8; x++) {
			if (ts_pixmap[tile_y + y][tile_x + x] != 0xF81F) {
				*px0++ = ts_pixmap[tile_y + y][tile_x + x];
				*px0++ = ts_pixmap[tile_y + y][tile_x + x];
				*px1++ = ts_pixmap[tile_y + y][tile_x + x];
				*px1++ = ts_pixmap[tile_y + y][tile_x + x];
			} else {
				px0+=2;
				px1+=2;
			}
		}
	}

	#if 0
	int tile_x = (tile_id * 8) % tile_video_state.tilesheet_w;
	int tile_y = ((tile_id * 8) / tile_video_state.tilesheet_w) * 8;

	for (int y = 0; y < 8; y++) {
		gfx_rgb565 *px0 =
			gfx_pixel_address_unchecked(slice, px, py + (y * 2));
		gfx_rgb565 *px1 =
			gfx_pixel_address_unchecked(slice, px, py + (y * 2) + 1);
		for (int x = 0; x < 8; x++) {
			int tile_pix =
				((tile_y + y) * tile_video_state.tilesheet_w) +
				(tile_x + x);
			*px0++ = tile_video_state.tilesheet[tile_pix];
			*px0++ = tile_video_state.tilesheet[tile_pix];
			*px1++ = tile_video_state.tilesheet[tile_pix];
			*px1++ = tile_video_state.tilesheet[tile_pix];
		}
	}
	#endif
}

void tile_animate(void)
{
#if 0
    tile_y += tile_y_inc;
    if (tile_y + (TS_PIXMAP_HEIGHT*2) > LCD_HEIGHT) {
        tile_y_inc = -1;
        tile_y -= 2;
    } else if (tile_y < 0) {
        tile_y_inc = +1;
        tile_y += 1;
    }
#endif
	static uint16_t delay = 0;
	static uint16_t stay = 0;

	if (delay == 1) {
		delay = 0;

		map_x_off += map_x_dir;
		if (map_x_off == 108*8) {
			map_x_dir = 0;
			stay++;
			if (stay == 10) {
				stay = 0;
				map_x_dir = -1;
			}
		}
		if (map_x_off == 0) {
			map_x_dir = 0;
			stay++;
			if (stay == 10) {
				stay = 0;
				map_x_dir = +1;
			}
		}
	} else {
		delay++;
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

#if 0
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
#endif

//	if(slice->y == 0) {
//		tile_draw_tile(slice, 1, 10, 10);
//	}

	for (size_t y = slice->y / 16; y < (slice->y + slice->h)/16; y++) {
		for (size_t x = 0; x < slice->w / 16; x++) {
			if (tml1_tilemap[0][y + 3][x + (map_x_off / 8)] != 0) {
				tile_draw_tile(slice, tml1_tilemap[0][y + 3][x + (map_x_off / 8)]-1, x * 16 - (map_x_off % 8) * 2, y*16);
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
