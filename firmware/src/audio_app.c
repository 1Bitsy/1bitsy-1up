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

#include "audio_app.h"

#include "audio.h"
#include "gamepad.h"
#include "lcd.h"
#include "math-util.h"
#include "text.h"
#include "volume.h"

// Define these to test other audio paths.
//#define MONO
//#define BIT8

#define Fs (44100.0)
#define FREQ0 (440.0 / 8) // A1, 55Hz
#define FREQ1 (523.3 / 8) // C2, 64 Hz

#ifdef MONO
    #define ACC  ACC_MONO
#else
    #define ACC  ACC_STEREO
#endif

#ifdef BIT8
    typedef uint8_t sample;
    #define SAMP_MAX 255
    #define ASD  ASD_8BIT
#else
    typedef uint16_t sample;
    #define SAMP_MAX 4095
    #define ASD  ASD_12BIT
#endif

const gfx_rgb565 bg_color  = 0xF81F;
const gfx_rgb565 vol_color = 0x07E0;

bool vol_is_visible;

char abuf[704] __attribute__((section(".sram2")));

#ifdef MONO

static void audio_callback(void *buf, size_t frame_count)
{
    sample *frames = buf;
    static float phase;

    for (size_t i = 0; i < frame_count; i++) {
        frames[i] = phase * SAMP_MAX;
        phase += FREQ0 / Fs;
        if (phase >= 1.0)
            phase -= 1.0;
    }
}

#else

static void audio_callback(void *buf, size_t frame_count)
{
    sample (*frames)[2] = buf;
    static float phase[2];

    for (size_t i = 0; i < frame_count; i++) {
        frames[i][0] = phase[0] * SAMP_MAX;
        frames[i][1] = SAMP_MAX - phase[1] * SAMP_MAX;
        phase[0] += FREQ0 / Fs;
        phase[1] += FREQ1 / Fs;
        if (phase[0] >= 1.0)
            phase[0] -= 1.0;
        if (phase[1] >= 1.0)
            phase[1] -= 1.0;
    }
}

#endif

void audio_app_init(void)
{
    audio_init(44100, ACC, ASD, abuf, sizeof abuf);
    audio_register_callback(audio_callback);
    audio_start();
}

void audio_app_end(void)
{
    audio_stop();
}

void audio_animate(void)
{
    vol_is_visible = true;
}

void audio_draw_vol(gfx_pixslice *slice)
{
    const int text_y = 12;
    const int bar_x = 32;
    const int bar_y = 13 * 16;
    const int bar_h = 16;

    if (!vol_is_visible) {
        return;
    }

    char *prefix = "vol: ";

   int xoff = 4;
    for (const char *p = prefix; *p; p++) {
        text_draw_char16(slice, *p, xoff, text_y, vol_color);
        xoff++;
    }

    uint16_t vol = volume_get();

     /* hundreds place */
    char h = vol < 100 ? ' ' : '0' + vol / 100 % 10;
    text_draw_char16(slice, h, xoff, text_y, vol_color);
    xoff++;

    /* tens place */
    char t = vol < 10 ? ' ' : '0' + vol / 10 % 10;
    text_draw_char16(slice, t, xoff, text_y, vol_color);
    xoff++;

    /* ones place */
    char o = '0' + vol / 1 % 10;
    text_draw_char16(slice, o, xoff, text_y, vol_color);
    xoff++;

    /* outline volume bar */
    for (int y = bar_y; y < bar_y + bar_h; y++) {
        gfx_rgb565 *px = gfx_pixel_address(slice, bar_x, y);
        px[0] = px[VOLUME_MAX] = vol_color;
    }
    gfx_rgb565 *px0 = gfx_pixel_address(slice, bar_x, bar_y);
    gfx_rgb565 *px1 = gfx_pixel_address(slice, bar_x, bar_y + bar_h - 1);
    if (px0) {
        for (int x = 1; x < VOLUME_MAX; x++) {
            px0[x] = vol_color;
        }
    }
    if (px1) {
        for (int x = 1; x < VOLUME_MAX; x++) {
            px1[x] = vol_color;
        }
    }

    /* fill volume bar */
    for (int y = bar_y + 1; y < bar_y + bar_h - 1; y++) {
        gfx_rgb565 *px = gfx_pixel_address(slice, bar_x, y);
        if (px) {
            for (int x = 0; x < vol; x++) {
                px[x] = vol_color;
            }
        }
    }
}

void audio_render_slice(gfx_pixslice *slice)
{
    audio_draw_vol(slice);
}

void audio_render(void)
{
    if (lcd_bg_color() != bg_color) {
        lcd_set_bg_color(bg_color, true);
    }

    for (size_t y = 0, h; y < LCD_HEIGHT; y += h) {
        h = MIN(LCD_MAX_SLICE_ROWS, LCD_HEIGHT - y);
        gfx_pixslice *slice = lcd_alloc_pixslice(0, y, LCD_WIDTH, h);
        audio_render_slice(slice);
        lcd_send_pixslice(slice);
    }
}
