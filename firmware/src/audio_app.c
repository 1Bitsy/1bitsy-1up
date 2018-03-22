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

const gfx_rgb565 bg_color  = 0xF81F;
const gfx_rgb565 vol_color = 0x07E0;

bool vol_is_visible;
uint16_t raw_vol;
uint16_t min_vol = UINT16_MAX;

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
#ifdef AUDIO_REPAIR
    pam8019_set_mode(PM_NORMAL);
#endif
    audio_start();
}

void audio_app_end(void)
{
    audio_stop();
#ifdef AUDIO_REPAIR
    pam8019_set_mode(PM_SHUTDOWN);
#endif
}

static void increment_volume(void)
{
#ifndef AUDIO_REPAIR
    uint8_t vol = volume_get();
    if (vol < VOLUME_MAX) {
        volume_set(vol + 1);
    }
#endif
}

static void decrement_volume(void)
{
#ifndef AUDIO_REPAIR
    uint8_t vol = volume_get();
    if (vol > 0) {
        volume_set(vol - 1);
    }
#endif
}

void audio_animate(void)
{
    uint16_t buttons = gamepad_get();
    static uint16_t prev_buttons;

    // Volume control is visible if button pressed in last 800 msec.
    const uint32_t VOL_FADE_TIME = 800;
    static uint32_t button_time;
    if (buttons & (GAMEPAD_BVOLP | GAMEPAD_BVOLM)) {
        button_time = system_millis;
    }
    vol_is_visible = system_millis - button_time < VOL_FADE_TIME;

    raw_vol = pam8019_get_raw_volume();
    if (min_vol > raw_vol)
        min_vol = raw_vol;

    if (buttons & GAMEPAD_BVOLP) {
        static uint32_t press_time, repeat_time, repeat_ivl;
        if (!(prev_buttons & GAMEPAD_BVOLP)) {
            // Initial button press.
            press_time = system_millis;
            repeat_ivl = 800;
            repeat_time = press_time + repeat_ivl;
            increment_volume();
        } else {
            if (((int32_t)repeat_time - (int32_t)system_millis) < 0) {
                // repeat button
                if (repeat_ivl > 50)
                    repeat_ivl /= 2;
                repeat_time += repeat_ivl;
                increment_volume();
            }
        }
    }
    if (buttons & GAMEPAD_BVOLM) {
        static uint32_t press_time, repeat_time, repeat_ivl;
        if (!(prev_buttons & GAMEPAD_BVOLM)) {
            // Initial button press.
            press_time = system_millis;
            repeat_ivl = 800;
            repeat_time = press_time + repeat_ivl;
            decrement_volume();
        } else {
            if (((int32_t)repeat_time - (int32_t)system_millis) < 0) {
                // repeat button
                if (repeat_ivl > 50)
                    repeat_ivl /= 2;
                repeat_time += repeat_ivl;
                decrement_volume();
            }
        }
    }
    prev_buttons = buttons;

#ifdef AUDIO_REPAIR

    if (button_pressed_debounce())
        pam8019_set_mode(PM_MUTED);
    else
        pam8019_set_mode(PM_NORMAL);

    switch (pam8019_get_mode()) {

    case PM_MUTED:
        led_set(system_millis & 0x80); // fast blink
        break;

    default:
        led_set(pam8019_output_is_headphones());
        break;
    }

#endif
}

static void audio_draw_vol(gfx_pixslice *slice)
{
#ifndef AUDIO_REPAIR
    const int text_y = 10;
    const int bar_x = 48;
    const int bar_y = 11 * 16;
    const int bar_h = 16;

    if (!vol_is_visible) {
        return;
    }

    char *prefix = "volume: ";

    int xoff = bar_x / 8;
    for (const char *p = prefix; *p; p++) {
        text_draw_char16(slice, *p, xoff, text_y, vol_color);
        xoff++;
    }

    uint8_t vol = volume_get();

    /* Draw "-NN dB". */
    if (vol == 0) {
        xoff = text_draw_str16(slice, "mute", xoff, text_y, vol_color);
    } else {
        int dB = VOLUME_MAX - (int)vol;
        assert(0 <= dB && dB < 100);
        if (dB < 10) {
            xoff++;
        }
        /* sign or blank for "0 dB" */
        if (dB != 0) {
            text_draw_char16(slice, '-', xoff, text_y, vol_color);
        }
        xoff++;
        if (dB >= 10) {
            /* tens place */
            char t = dB / 10 + '0';
            text_draw_char16(slice, t, xoff, text_y, vol_color);
            xoff++;
        }
        char o = '0' + dB % 10;
        text_draw_char16(slice, o, xoff, text_y, vol_color);
        xoff++;

        /* "dB" */
        xoff++;
        xoff = text_draw_str16(slice, "dB", xoff, text_y, vol_color);
    }

    /* outline volume bar */
    for (int y = bar_y; y < bar_y + bar_h; y++) {
        gfx_rgb565 *px = gfx_pixel_address(slice, bar_x, y);
        px[0] = px[VOLUME_MAX * 6] = vol_color;
    }
    gfx_rgb565 *px0 = gfx_pixel_address(slice, bar_x, bar_y);
    gfx_rgb565 *px1 = gfx_pixel_address(slice, bar_x, bar_y + bar_h - 1);
    if (px0) {
        for (int x = 6 * vol + 1; x < 6 * VOLUME_MAX; x++) {
            px0[x] = vol_color;
        }
    }
    if (px1) {
        for (int x = 6 * vol + 1; x < 6 * VOLUME_MAX; x++) {
            px1[x] = vol_color;
        }
    }

    /* fill volume bar */
    for (int y = bar_y; y < bar_y + bar_h; y++) {
        gfx_rgb565 *px = gfx_pixel_address(slice, bar_x, y);
        if (px) {
            for (int x = 0; x < vol * 6; x++) {
                if (x % 6) {
                    px[x] = vol_color;
                }
            }
        }
    }
#endif
}

static void audio_draw_raw_vol(gfx_pixslice *slice)
{
//#ifndef AUDIO_REPAIR
    const int text_y = 12;
    // const int bar_x = 32;
    // const int bar_y = 13 * 16;
    // const int bar_h = 16;

    // if (!vol_is_visible) {
    //     return;
    // }

    char *prefix = "raw vol: ";

    int xoff = 4;
    for (const char *p = prefix; *p; p++) {
        text_draw_char16(slice, *p, xoff, text_y, vol_color);
        xoff++;
    }

    // uint16_t vol = volume_get_raw();

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

#ifndef AUDIO_REPAIR
    /* outline volume bar */
    for (int y = bar_y; y < bar_y + bar_h; y++) {
        gfx_rgb565 *px = gfx_pixel_address(slice, bar_x, y);
        px[0] = px[VOLUME_RAW_MAX] = vol_color;
    }
    gfx_rgb565 *px0 = gfx_pixel_address(slice, bar_x, bar_y);
    gfx_rgb565 *px1 = gfx_pixel_address(slice, bar_x, bar_y + bar_h - 1);
    if (px0) {
        for (int x = 1; x < VOLUME_RAW_MAX; x++) {
            px0[x] = vol_color;
        }
    }
    if (px1) {
        for (int x = 1; x < VOLUME_RAW_MAX; x++) {
            px1[x] = vol_color;
        }
    }

    /* fill volume bar */
    for (int y = bar_y + 1; y < bar_y + bar_h - 1; y++) {
        gfx_rgb565 *px = gfx_pixel_address(slice, bar_x, y);
        if (px) {
            for (int x = 0; x < raw_vol; x++) {
                px[x] = vol_color;
            }
        }
    }
#endif
}

void audio_render_slice(gfx_pixslice *slice)
{
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
