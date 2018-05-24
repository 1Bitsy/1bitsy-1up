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

#include <assert.h>

#include "audio.h"
#include "button_boot.h"
#include "gamepad.h"
#include "lcd.h"
#include "led.h"
#include "math-util.h"
#include "pam8019.h"
#include "systick.h"
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

typedef enum text_row {
    TR_HEADER = 3,
    TR_INSTR  = 5,
    TR_MODE   = 7,
    TR_OUTPUT = 8,
    TR_VOL    = 10,
    TR_VOLBAR = 11,
    TR_RAWVOL = 12,
    TR_RVBAR  = 13,
} text_row;

typedef enum text_col {
    TC_LEFT   = 4,
} text_col;

static const gfx_rgb565 bg_color  = 0xF81F;
static const gfx_rgb565 vol_color = 0x07E0;

static bool vol_is_visible;
static pam8019_mode cur_mode;
static uint8_t vol;
static uint8_t prev_vol;
static uint16_t raw_vol;

// char abuf[704] __attribute__((section(".sram2")));
#define FRAMES ((int)Fs / 250)
DEFINE_AUDIO_BUFFER(abuf, FRAMES, ACC, ASD);

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
    pam8019_set_mode(PM_NORMAL);
    audio_start();
    prev_vol = volume_get();
}

void audio_app_end(void)
{
    audio_stop();
    pam8019_set_mode(PM_SHUTDOWN);
}

void audio_animate(void)
{
    // Volume control is visible if volume changed in last 800 msec.

    const uint32_t VOL_FADE_TIME = 800;
    static uint32_t vol_change_time = 0xF0000000; // large negative

    cur_mode = pam8019_get_mode();
    vol = volume_get();
    raw_vol = volume_get_raw();

    if (prev_vol != vol) {
        prev_vol = vol;
        vol_change_time = system_millis;
    }
    vol_is_visible = system_millis - vol_change_time < VOL_FADE_TIME;

    uint16_t buttons = gamepad_get();
    static uint16_t prev_buttons;
    
    if ((buttons & ~prev_buttons) & GAMEPAD_BSELECT) {
    
        cur_mode = pam8019_get_mode() + 1;
        if (cur_mode == PM_END)
            cur_mode = PM_START;
        pam8019_set_mode(cur_mode);
    }
    prev_buttons = buttons;
}

static void audio_draw_vol(gfx_pixslice *slice)
{
    const int text_y = TR_VOL;
    const int text_x = TC_LEFT;
    const int bar_x = 8 * text_x;
    const int bar_y = TR_VOLBAR * 16;
    const int bar_h = 15;

    if (!vol_is_visible) {
        return;
    }


    int xoff = text_x;
    xoff = text_draw_str16(slice, "Volume: ", xoff, text_y, vol_color);

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
        px[0] = px[VOLUME_MAX * 4] = vol_color;
    }
    gfx_rgb565 *px0 = gfx_pixel_address(slice, bar_x, bar_y);
    gfx_rgb565 *px1 = gfx_pixel_address(slice, bar_x, bar_y + bar_h - 1);
    if (px0) {
        for (int x = 4 * vol + 1; x < 4 * VOLUME_MAX; x++) {
            px0[x] = vol_color;
        }
    }
    if (px1) {
        for (int x = 4 * vol + 1; x < 4 * VOLUME_MAX; x++) {
            px1[x] = vol_color;
        }
    }

    /* fill volume bar */
    for (int y = bar_y; y < bar_y + bar_h; y++) {
        gfx_rgb565 *px = gfx_pixel_address(slice, bar_x, y);
        if (px) {
            for (int x = 0; x < 4 * vol; x++) {
                if (x % 6) {
                    px[x] = vol_color;
                }
            }
        }
    }
}

static void audio_draw_raw_vol(gfx_pixslice *slice)
{
    const int text_y = TR_RAWVOL;
    const int text_x = TC_LEFT;
    const int bar_x = 8 * text_x;
    const int bar_y = TR_RVBAR * 16;
    const int bar_h = 15;

    if (!vol_is_visible) {
        return;
    }

    char *prefix = "raw vol: ";

    int xoff = text_x;
    for (const char *p = prefix; *p; p++) {
        text_draw_char16(slice, *p, xoff, text_y, vol_color);
        xoff++;
    }

    /* thousands place */
    char k = raw_vol < 1000 ? ' ' : '0' + raw_vol / 1000 % 10;
    text_draw_char16(slice, k, xoff, text_y, vol_color);
    xoff++;
 
    /* hundreds place */
    char h = raw_vol < 100 ? ' ' : '0' + raw_vol / 100 % 10;
    text_draw_char16(slice, h, xoff, text_y, vol_color);
    xoff++;
 
    /* tens place */
    char t = raw_vol < 10 ? ' ' : '0' + raw_vol / 10 % 10;
    text_draw_char16(slice, t, xoff, text_y, vol_color);
    xoff++;

    /* ones place */
    char o = '0' + raw_vol / 1 % 10;
    text_draw_char16(slice, o, xoff, text_y, vol_color);
    xoff++;

    /* outline volume bar */
    for (int y = bar_y; y < bar_y + bar_h; y++) {
        gfx_rgb565 *px = gfx_pixel_address(slice, bar_x, y);
        px[0] = px[VOLUME_RAW_MAX >> 4] = vol_color;
    }
    gfx_rgb565 *px0 = gfx_pixel_address(slice, bar_x, bar_y);
    gfx_rgb565 *px1 = gfx_pixel_address(slice, bar_x, bar_y + bar_h - 1);
    if (px0) {
        for (int x = 1; x < VOLUME_RAW_MAX >> 4; x++) {
            px0[x] = vol_color;
        }
    }
    if (px1) {
        for (int x = 1; x < VOLUME_RAW_MAX >> 4; x++) {
            px1[x] = vol_color;
        }
    }

    /* fill volume bar */
    for (int y = bar_y + 1; y < bar_y + bar_h - 1; y++) {
        gfx_rgb565 *px = gfx_pixel_address(slice, bar_x, y);
        if (px) {
            for (int x = 0; x < raw_vol >> 4; x++) {
                px[x] = vol_color;
            }
        }
    }
}

static void audio_draw_header(gfx_pixslice *slice)
{
    int text_y = TR_HEADER;
    int text_x = TC_LEFT;
    text_draw_str16(slice, "Audio Saw Test", text_x, text_y, vol_color);
}

static void audio_draw_instructions(gfx_pixslice *slice)
{
    int text_y = TR_INSTR;
    int text_x = TC_LEFT;
    text_draw_str16(slice, "Press SELECT to change mode.",
                    text_x, text_y, vol_color);
}

static void audio_draw_mode(gfx_pixslice *slice)
{
    const char *mode_str;
    switch (cur_mode) {
    case PM_SHUTDOWN:
        mode_str = "Shutdown";
        break;

    case PM_MUTED:
        mode_str = "Muted";
        break;

    case PM_NORMAL:
        mode_str = "Normal";
        break;

    case PM_OVERRIDE_HP:
        mode_str = "Force Headphones";
        break;

    case PM_OVERRIDE_SPKR:
        mode_str = "Force Speaker";
        break;

    default:
        assert(false);
    }
    int text_y = TR_MODE;
    int text_x = TC_LEFT;
    int xoff = text_draw_str16(slice, "Mode:    ", text_x, text_y, vol_color);
    xoff = text_draw_str16(slice, mode_str, xoff, text_y, vol_color);

}

static void audio_draw_output(gfx_pixslice *slice)
{
    int text_y = TR_OUTPUT;
    int text_x = TC_LEFT;
    int xoff = text_draw_str16(slice, "Output:  ", text_x, text_y, vol_color);
    const char *out = pam8019_output_is_headphones() ? "Headphones" : "Speaker";
    xoff = text_draw_str16(slice, out, xoff, text_y, vol_color);
}

static void audio_render_slice(gfx_pixslice *slice)
{
    audio_draw_header(slice);
    audio_draw_instructions(slice);
    audio_draw_mode(slice);
    audio_draw_output(slice);
    audio_draw_vol(slice);
    audio_draw_raw_vol(slice);
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
