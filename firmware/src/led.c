#include "led.h"

#include "gpio.h"

static const gpio_pin led_pin = {
    .gp_port = GPIOA,
    .gp_pin  = GPIO8,
    .gp_mode = GPIO_MODE_OUTPUT,
    .gp_level = 1,
};

void led_init(void)
{
    gpio_init_pin(&led_pin);
}

bool led_get(void)
{
    return !gpio_get(led_pin.gp_port, led_pin.gp_pin);
}

void led_set(bool on_off)
{
    if (on_off) {
        led_on();
    } else {
        led_off();
    }
}

void led_on(void)
{
    gpio_clear(led_pin.gp_port, led_pin.gp_pin);
}

void led_off(void)
{
    gpio_set(led_pin.gp_port, led_pin.gp_pin);
}

void led_toggle(void)
{
    gpio_toggle(led_pin.gp_port, led_pin.gp_pin);
}
