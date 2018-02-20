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

#include <libopencm3/stm32/gpio.h>
#include <string.h>

#include "tile_app.h"

#include "gamepad.h"
#include "lcd.h"
#include "math-util.h"
#include "text.h"

#include "assets/assets.h"

gfx_rgb565 color = 0;

static int16_t map_x_off = 0;
static int16_t map_x_dir = +1;
static int16_t map_y_off = 3 * 8;

static int16_t spr_x = 10;
static int16_t spr_y = 10;
static int16_t spr_x_dir = +1;
static int16_t spr_y_dir = +1;

static struct tile_video_state {
	uint16_t const *tilemap;
	uint16_t tilemap_w;
	uint16_t tilemap_h;
	uint16_t const *tilesheet;
	uint16_t tilesheet_w;
	uint16_t tilesheet_h;
	uint16_t x_off;
	uint16_t y_off;
	uint32_t frames; /* just a counter of frames since time began. */
	uint32_t idle; /* time since no gamepad input */
} tile_video_state;

/* Selects type of the sprite including fully disabled. */
enum sprite_type {
	S_off,
	S_8x8,
	S_8x16,
	S_16x8,
	S_16x16,
};

/* Sprite attributes. */
/* We are using up to 4 tiles from the sprite sheet to represent a single
 * sprite object. */
struct sprite {
	uint16_t tiles[4];
	enum sprite_type type;
	uint16_t x;
	uint16_t y;
};

#define SPRITE_MAX_COUNT 16

struct sprite sprites[SPRITE_MAX_COUNT];

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

	tile_video_state.frames = 0; /* reset time */

	for(int i = 0; i < SPRITE_MAX_COUNT; i++) {
		sprites[i].type = S_off;
	}

	sprites[0].type = S_8x16;
	sprites[0].tiles[0] = 22;
	sprites[0].tiles[1] = 64;
	sprites[0].x = 10;
	sprites[0].y = 10;
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

	int tile_x0 = 0;
	int tile_w = 8;
	if (px < 0) {
		tile_x0 += -px / 2;
		px = 0;
	}
	else if (px + 16 > LCD_WIDTH) {
		tile_w -= (px + 16 - LCD_WIDTH) / 2;
	}

	int tile_y0 = 0;
	int tile_h = 8;
	if (py < 0) {
		tile_y0 += -py / 2;
		py = 0;
	}
	else if (py + 16 > LCD_HEIGHT) {
		tile_h -= (py + 16 - LCD_HEIGHT) / 2;
	}


	for (int y = tile_y0; y < tile_h; y++) {
		gfx_rgb565 *px0 =
			gfx_pixel_address(slice, px, py + ((y - tile_y0) * 2));
		gfx_rgb565 *px1 =
			gfx_pixel_address(slice, px, py + ((y - tile_y0) * 2) + 1);

		for (int x = tile_x0; x < tile_w; x++) {
			uint16_t c = ts_pixmap[tile_y + y][tile_x + x];

			if (c != 0xF81F) {
				if (px0) {
					*px0++ = c;
					*px0++ = c;
				}
				if (px1) {
					*px1++ = c;
					*px1++ = c;
				}
			} else {
				if (px0) px0+=2;
				if (px1) px1+=2;
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

void tile_draw_sprite_tile(gfx_pixslice *slice, uint16_t tile_id, int px, int py)
{
	int tile_x = (tile_id * 8) % SS_PIXMAP_WIDTH;
	int tile_y = ((tile_id * 8) / SS_PIXMAP_WIDTH) * 8;

	/* Check if the tile exists. */
	if (tile_x >= SS_PIXMAP_WIDTH)
	{
		return;
	}

	if (tile_y >= SS_PIXMAP_HEIGHT)
	{
		return;
	}

	/* Trying to draw out of bounds. */
	if ((px < -16) ||
	    (px >= LCD_WIDTH) ||
	    (py < (slice->y - 16)) ||
	    (py >= (slice->y + slice->h))) {
		return;
	}

	/* Trim draw bounds. */
	int tile_x0 = 0;
	int tile_w = 8;
	if (px < 0) {
		tile_x0 += -px / 2;
		px = 0;
	}
	else if (px + 16 > LCD_WIDTH) {
		tile_w -= (px + 16 - LCD_WIDTH) / 2;
	}

	int tile_y0 = 0;
	int tile_h = 8;
	if (py < 0) {
		tile_y0 += -py / 2;
		py = 0;
	}
	else if (py + 16 > LCD_HEIGHT) {
		tile_h -= (py + 16 - LCD_HEIGHT) / 2;
	}

	/* Draw tile. */
	for (int y = tile_y0; y < tile_h; y++) {
		gfx_rgb565 *px0 =
			gfx_pixel_address(slice, px, py + ((y - tile_y0) * 2));
		gfx_rgb565 *px1 =
			gfx_pixel_address(slice, px, py + ((y - tile_y0) * 2) + 1);

		for (int x = tile_x0; x < tile_w; x++) {
			uint16_t c = ss_pixmap[tile_y + y][tile_x + x];

			if (c != 0xF81F) {
				if (px0) {
					*px0++ = c;
					*px0++ = c;
				}
				if (px1) {
					*px1++ = c;
                                        *px1++ = c;
				}
			} else {
				if (px0) px0+=2;
				if (px1) px1+=2;
			}
		}
	}
}

void tile_draw_sprite(gfx_pixslice *slice, struct sprite *sprite)
{
	switch(sprite->type) {
		case S_off:
			return;
		case S_8x8:
			tile_draw_sprite_tile(slice, sprite->tiles[0],
					 sprite->x, sprite->y);
			break;
		case S_8x16:
			tile_draw_sprite_tile(slice, sprite->tiles[0],
					 sprite->x, sprite->y);
			tile_draw_sprite_tile(slice, sprite->tiles[1],
					 sprite->x, sprite->y + 16);
			break;
		case S_16x8:
			tile_draw_sprite_tile(slice, sprite->tiles[0],
					 sprite->x, sprite->y);
			tile_draw_sprite_tile(slice, sprite->tiles[1],
					 sprite->x + 16, sprite->y);
			break;
		case S_16x16:
			tile_draw_sprite_tile(slice, sprite->tiles[0],
					 sprite->x, sprite->y);
			tile_draw_sprite_tile(slice, sprite->tiles[1],
					 sprite->x + 16, sprite->y);
			tile_draw_sprite_tile(slice, sprite->tiles[2],
					 sprite->x, sprite->y + 16);
			tile_draw_sprite_tile(slice, sprite->tiles[3],
					 sprite->x + 16, sprite->y + 16);
			break;
	}
}

void tile_draw_sprites(gfx_pixslice *slice)
{
	for(int i=SPRITE_MAX_COUNT; i > 0; i--) {
		tile_draw_sprite(slice, &sprites[i-1]);
	}
}

void tile_draw_char8(gfx_pixslice *slice, char ch, int x, int y, gfx_rgb565 color)
{
	int xoff = x * 4;
	int yoff = y * 8;

	if ((ch < MINIWI_FONT_OFFSET) || (ch >= (MINIWI_FONT_OFFSET + MINIWI_FONT_GLYPH_COUNT))) {
		return;
	}

	for (int py = 0; py < 8; py++) {
		gfx_rgb565 *px0 =
			gfx_pixel_address_unchecked(slice, xoff, yoff + py);
		for (int px = 0; px < 4; px++) {
			if ((miniwi_font[(int)ch - MINIWI_FONT_OFFSET][px] & (1 << py)) != 0) {
				*px0++ = color;
			} else {
				px0++;
			}
		}
	}
}

extern uint32_t fps;

void tile_draw_fps(gfx_pixslice *slice)
{
	if (slice->y >= 16)
		return;

	uint32_t lfps = fps;
	int pos = 0;
	char *prefix = "fps: ";

	for (; lfps > 0; lfps /= 10) {
		//tile_draw_char8(slice, (lfps % 10) + '0', LCD_WIDTH / 4 - (pos + 1), 0, 0x0000);
		text_draw_char16(slice, (lfps % 10) + '0', LCD_WIDTH / 8 - (pos + 1), 0, 0x0000);
		pos++;
	}

	for (int i = 0; i < 5; i++) {
		//tile_draw_char8(slice, prefix[4 - i], LCD_WIDTH / 4 - (pos + 1), 0, 0x0000);
		text_draw_char16(slice, prefix[4 - i], LCD_WIDTH / 8 - (pos + 1), 0, 0x0000);
		pos++;
	}
}

void tile_draw_gamepad(gfx_pixslice *slice)
{
	if (slice->y >= 32)
		return;

	char *prefix = "gamepad: ";
	uint16_t gamepad = gamepad_get();

	for (int i = 0; i < 16; i++) {
		if ((gamepad & (1 << i)) != 0) {
			//tile_draw_char8(slice, '1', LCD_WIDTH / 4 - (i + 1), 1, 0x0000);
			text_draw_char16(slice, '1', LCD_WIDTH / 8 - (i + 1), 1, 0x0000);
		} else {
			//tile_draw_char8(slice, '0', LCD_WIDTH / 4 - (i + 1), 1, 0x0000);
			text_draw_char16(slice, '0', LCD_WIDTH / 8 - (i + 1), 1, 0x0000);
		}
	}

	for (int i = 0; prefix[i] != 0; i++) {
		//tile_draw_char8(slice, prefix[i],
		//	LCD_WIDTH / 4 - (16 + strlen(prefix)) + i, 1, 0x0000);
		text_draw_char16(slice, prefix[i],
			LCD_WIDTH / 8 - (16 + strlen(prefix)) + i, 1, 0x0000);
	}
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
	uint16_t gamepad = gamepad_get();

	if (gamepad != 0x0000) {
		tile_video_state.idle = 0;
	}

	if ((gamepad != 0xFFFF) && (tile_video_state.idle < (80 * 10))) {
		if ((gamepad & GAMEPAD_BLEFT) != 0) {
			if (map_x_off > 0) {
				map_x_off--;
			}
		} else if ((gamepad & GAMEPAD_BRIGHT) != 0) {
			if (map_x_off < (108 * 8)) {
				map_x_off++;
			}
		}
		if ((gamepad & GAMEPAD_BUP) != 0) {
			if (map_y_off > 0) {
				map_y_off--;
			}
		} else if ((gamepad & GAMEPAD_BDOWN) != 0) {
			if (map_y_off < (3 * 8)) {
				map_y_off++;
			}
		}

		if ((gamepad & GAMEPAD_BY) != 0) {
			if (spr_x > 0) {
				spr_x--;
			}
		} else if ((gamepad & GAMEPAD_BA) != 0) {
			if (spr_x < (19 * 8 * 2)) {
				spr_x++;
			}
		}
		if ((gamepad & GAMEPAD_BX) != 0) {
			if (spr_y > 0) {
				spr_y--;
			}
		} else if ((gamepad & GAMEPAD_BB) != 0) {
			if (spr_y < (13 * 8 * 2)) {
				spr_y++;
			}
		}
	} else {
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

		spr_x += spr_x_dir;
		spr_y += spr_y_dir;

		if ((spr_x + 14) == (LCD_WIDTH-1)) {
			spr_x_dir = -1;
		}

		if (spr_x == 0) {
			spr_x_dir = 1;
		}

		if ((spr_y + 22) == (LCD_HEIGHT-1)) {
			spr_y_dir = -1;
		}

		if (spr_y == 0) {
			spr_y_dir = 1;
		}
	}

	if ((tile_video_state.frames / 40) % 2) { /* every 80 frames */
		sprites[0].tiles[0] = 43;
		sprites[0].tiles[1] = 64;
	} else {
		sprites[0].tiles[0] = 21;
		sprites[0].tiles[1] = 63;
	}

	sprites[0].x = spr_x;
	sprites[0].y = spr_y;
}

static void tile_render_slice(gfx_pixslice *slice)
{

#if 0
	/* Draw some lines to indicate drawing direction and pixslice start/end */
	XXX must check pixel address.  NULL is out of bounds.
	gfx_rgb565 *px =
		gfx_pixel_address_unchecked(slice, 0, slice->y);
	for (size_t x = 0; x < 10; x++) {
		*px++ = 0xFFFF;
	}
	XXX must check pixel address.  NULL is out of bounds.
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
		XXX must check pixel address.  NULL is out of bounds.
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

	/* gpio_set(GPIOA, GPIO3); */

	/* Tilemap */
	for (size_t y = slice->y / 16; y <= (slice->y + slice->h)/16; y++) {
		for (size_t x = 0; x <= slice->w / 16; x++) {
			const uint16_t tid =
				tml1_tilemap[0][y + map_y_off / 8][x + map_x_off / 8];
			if (tid) {
				tile_draw_tile(slice,
				               tid - 1,
				               x * 16 - (map_x_off % 8) * 2,
				               y * 16 - (map_y_off % 8) * 2);
			}
		}
	}

	/* Sprites */
	tile_draw_sprites(slice);

	/* Text */
	if(slice->y == 0) {
		tile_draw_fps(slice);
		tile_draw_gamepad(slice);
	}

	/* gpio_clear(GPIOA, GPIO3); */
}

void tile_render(void)
{
	size_t h;
        size_t slice_max_h = LCD_MAX_SLICE_ROWS;
	color = 0;

	if (lcd_bg_color() != TML1_TILEMAP_BG_COLOR) {
		lcd_set_bg_color(TML1_TILEMAP_BG_COLOR, true);
	}

    for (size_t y = 0; y < LCD_HEIGHT; y += h) {
        h = MIN(slice_max_h, LCD_HEIGHT - y);
        gfx_pixslice *slice = lcd_alloc_pixslice(0, y, LCD_WIDTH, h);
        tile_render_slice(slice);
        lcd_send_pixslice(slice);
    }

    tile_video_state.frames++; /* increment time */
    tile_video_state.idle++; /* increment idle time */
}
