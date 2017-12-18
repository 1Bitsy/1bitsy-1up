/*
 * This file is part of the 1Bitsy 1UP retro inspired game console project.
 *
 * Copyright (C) 2017 Bob Miller <kbob@jogger-egg.com>
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

#include <stdbool.h>

#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "button_boot.h"
#include "gamepad.h"
#include "touch.h"
#include "i2c.h"
#include "lcd.h"
#include "math-util.h"
#include "volume.h"
#include "systick.h"
#include "text.h"

/* "Apps" */
#include "munch_app.h"
#include "tile_app.h"
#include "audio_app.h"

#define MY_CLOCK (rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_168MHZ])
#define BG_COLOR 0x0000         // black

static const i2c_config i2c_cfg = {
    .i_base_address    = I2C1,
    .i_own_address     = 32,
    .i_pins = {
        {                   // SCL: pin PB6, AF4
            .gp_port   = GPIOB,
            .gp_pin    = GPIO6,
            .gp_mode   = GPIO_MODE_AF,
            .gp_pupd   = GPIO_PUPD_NONE,
            .gp_af     = GPIO_AF4,
            .gp_ospeed = GPIO_OSPEED_50MHZ,
            .gp_otype  = GPIO_OTYPE_OD,
            .gp_level  = 1,

        },
        {                   // SDA: pin PB7, AF4
            .gp_port   = GPIOB,
            .gp_pin    = GPIO7,
            .gp_mode   = GPIO_MODE_AF,
            .gp_pupd   = GPIO_PUPD_NONE,
            .gp_af     = GPIO_AF4,
            .gp_ospeed = GPIO_OSPEED_50MHZ,
            .gp_otype  = GPIO_OTYPE_OD,
            .gp_level  = 1,
        },
    },
};

uint32_t   fps;

enum app_ids {
    munch_app,
    tile_app,
    audio_app,
    end_app
} active_app = munch_app;

/* Function pointers to available apps. The first one executes by default.
 * Do not forget to adjust the enum if you are messing around with this list.
 */
struct app {
	void (*init)(void);
	void (*animate)(void);
	void (*render)(void);
	void (*end)(void);
} apps[] = {
	[munch_app] = {
		.init = munch_init,
		.animate = munch_animate,
		.render = munch_render,
                .end = NULL,
	},
	[tile_app] = {
		.init = tile_init,
		.animate = tile_animate,
		.render = tile_render,
                .end = NULL,
	},
	[audio_app] = {
		.init = audio_app_init,
		.animate = audio_animate,
		.render = audio_render,
                .end = audio_app_end,
	},
};

static void handle_systick(uint32_t millis)
{
    button_pressed_debounce();
}

static void setup(void)
{
    rcc_clock_setup_hse_3v3(&MY_CLOCK);
    flash_prefetch_enable();
    flash_icache_enable();
    flash_dcache_enable();

    setup_systick(MY_CLOCK.ahb_frequency);
    register_systick_handler(handle_systick);

    lcd_set_bg_color(BG_COLOR, false);
    lcd_init();

    i2c_init(&i2c_cfg);
    volume_init();

    text_init();

    gamepad_init();
    touch_init();

    /* Toggles with every frame */
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
    /* Is high when a slice is being rendered by the app. */
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
}

static void calc_fps(void)
{
    static uint32_t next_time;
    static uint32_t frame_count;
    frame_count++;
    gpio_toggle(GPIOA, GPIO1);
    if (system_millis >= next_time) {
        fps = frame_count;
        frame_count = 0;
        next_time += 1000;
    }
}

static void check_app_switch(void)
{
    static bool state = false;

    if (!state) {
        if (button_pressed_debounce()) {
            if (apps[active_app].end) {
                apps[active_app].end();
            }
            active_app++;
            if (active_app == end_app) {
                active_app = 0;
            }
            if (apps[active_app].init) {
                apps[active_app].init();
            }
            state = true;
            gpio_clear(GPIOA, GPIO8);
        }
    } else {
        if (button_released_debounce()) {
            state = false;
            gpio_set(GPIOA, GPIO8);
        }
    }
}

static void run(void)
{
    if (apps[active_app].init) {
        apps[active_app].init();
    }
    while (true) {
        apps[active_app].animate();
        apps[active_app].render();
        check_app_switch();
        calc_fps();
    }
}

int main(void)
{
    /* Launch the ROM DFU bootloader if the user button is pressed just after
     * reset.
     */
    button_boot();

    setup();
    run();
}

