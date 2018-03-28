#include "pam8019.h"

#include <assert.h>

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/rcc.h>

#include "gpio.h"

#define HP_SENSE_PORT GPIOA
#define HP_SENSE_PIN  GPIO0
#define VOL_PORT      GPIOA
#define VOL_PIN       GPIO1
#define VOL_ADC_CHAN  1
#define MUTE_PORT     GPIOA
#define MUTE_PIN      GPIO2
#define SHUTDOWN_PORT GPIOA
#define SHUTDOWN_PIN  GPIO3


static const gpio_pin pam8019_pins[] = {
    {                           // HP SENSE (bidirectional, initially input)
        .gp_port  = HP_SENSE_PORT,
        .gp_pin   = HP_SENSE_PIN,
        .gp_mode  = GPIO_MODE_INPUT,
    },
    {                           // VOL_ADC
        .gp_port  = VOL_PORT,
        .gp_pin   = VOL_PIN,
        .gp_mode  = GPIO_MODE_ANALOG,
    },
    {                           // MUTE
        .gp_port  = MUTE_PORT,
        .gp_pin   = MUTE_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
        .gp_level = 0,
    },
    {                           // SHUTDOWN
        .gp_port  = SHUTDOWN_PORT,
        .gp_pin   = SHUTDOWN_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
        .gp_level = 0,          // default is shut down.
    },
};

static const size_t pam8019_pin_count = (&pam8019_pins)[1] - pam8019_pins;

static void enable_override(void)
{
#if 0
    gpio_mode_setup(HP_SENSE_PORT,
                    GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE,
                    HP_SENSE_PIN);
#else
    static const gpio_pin sense_output = {
        .gp_port  = HP_SENSE_PORT,
        .gp_pin   = HP_SENSE_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
        .gp_otype = GPIO_OTYPE_PP,
    };
    gpio_config_pin(&sense_output);
#endif
}

static void disable_override(void)
{
#if 0
    gpio_mode_setup(HP_SENSE_PORT,
                    GPIO_MODE_INPUT,
                    GPIO_PUPD_NONE,
                    HP_SENSE_PIN);
#else
    static const gpio_pin sense_input = {
        .gp_port  = HP_SENSE_PORT,
        .gp_pin   = HP_SENSE_PIN,
        .gp_mode  = GPIO_MODE_INPUT,
    };
    gpio_config_pin(&sense_input);
#endif
}

void pam8019_init(void)
{
    gpio_init_pins(pam8019_pins, pam8019_pin_count);

    rcc_periph_clock_enable(RCC_ADC1);

    adc_power_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);
    adc_power_on(ADC1);
}

pam8019_mode pam8019_get_mode(void)
{
    if (!gpio_get(SHUTDOWN_PORT, SHUTDOWN_PIN))
        return PM_SHUTDOWN;

    if (gpio_get(MUTE_PORT, MUTE_PIN))
        return PM_MUTED;

    uint32_t mode = GPIO_MODER(HP_SENSE_PORT);
    mode /= HP_SENSE_PIN * HP_SENSE_PIN;
    if ((mode & 0x03) == GPIO_MODE_INPUT)
        return PM_NORMAL;

    if (gpio_get(HP_SENSE_PORT, HP_SENSE_PIN))
        return PM_OVERRIDE_HP;
    else
        return PM_OVERRIDE_SPKR;
}

void pam8019_set_mode(pam8019_mode new_mode)
{
    // N.B., SHUTDOWN is active low.  MUTE is active high.

    switch (new_mode) {

    case PM_NORMAL:
        gpio_set(SHUTDOWN_PORT, SHUTDOWN_PIN);
        gpio_clear(MUTE_PORT, MUTE_PIN);
        disable_override();
        break;

    case PM_SHUTDOWN:
        gpio_clear(SHUTDOWN_PORT, SHUTDOWN_PIN);
        break;

    case PM_MUTED:
        gpio_set(SHUTDOWN_PORT, SHUTDOWN_PIN);
        gpio_set(MUTE_PORT, MUTE_PIN);
        break;

    case PM_OVERRIDE_HP:
        gpio_set(SHUTDOWN_PORT, SHUTDOWN_PIN);
        gpio_clear(MUTE_PORT, MUTE_PIN);
        enable_override();
        gpio_set(HP_SENSE_PORT, HP_SENSE_PIN);
        break;

    case PM_OVERRIDE_SPKR:
        gpio_set(SHUTDOWN_PORT, SHUTDOWN_PIN);
        gpio_clear(MUTE_PORT, MUTE_PIN);
        enable_override();
        gpio_clear(HP_SENSE_PORT, HP_SENSE_PIN);
        break;

    default:
        assert(false);
    }
}

bool pam8019_output_is_headphones(void)
{
    return (bool)gpio_get(HP_SENSE_PORT, HP_SENSE_PIN);
}

uint16_t pam8019_get_raw_volume(void)
{
    uint8_t channel_array[16] = { VOL_ADC_CHAN };
    adc_set_regular_sequence(ADC1, 1, channel_array);
    adc_start_conversion_regular(ADC1);
    while (!adc_eoc(ADC1))
        continue;
    return 4095 - adc_read_regular(ADC1);
}
