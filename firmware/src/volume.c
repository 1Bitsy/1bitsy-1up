#include "volume.h"

#ifdef AUDIO_REPAIR

#include "pam8019.h"

static uint16_t volume_raw_min = 600; // raw volume doesn't go below 570-ish.

static uint16_t smooth(uint32_t numer, uint32_t denom, uint16_t expected)
{
    if (numer + denom / 2 >= expected * denom &&
        numer - denom / 2 <= (expected + 1) * denom)
        return expected;
    return numer / denom;
}

void volume_init(void)
{
}

uint8_t volume_get(void)
{
    uint16_t raw = volume_get_raw();

    if (volume_raw_min > raw)
        volume_raw_min = raw;
    uint32_t numerator = raw - volume_raw_min;
    uint32_t denominator = VOLUME_RAW_MAX - volume_raw_min;
    numerator *= VOLUME_MAX - VOLUME_MIN + 1;

    static uint16_t smoothed_vol = UINT16_MAX;
    smoothed_vol = smooth(numerator, denominator, smoothed_vol);
    // smoothed_vol = numerator / denominator;
    

    return smoothed_vol;
}

uint16_t volume_get_raw(void)
{
    return pam8019_get_raw_volume();
}

#else /* !AUDIO_REPAIR */

#include <assert.h>

#include <libopencm3/stm32/i2c.h>

#include "i2c.h"
#include "systick.h"            // XXX

enum mcp4XXX_registers {
    REG_VOLATILE_WIPER_0    = 0,
    REG_VOLATILE_WIPER_1    = 1,
    REG_NONVOLATILE_WIPER_0 = 2,
    REG_NONVOLATILE_WIPER_1 = 3,
    REG_VOLATILE_TCON       = 4,
    REG_STATUS              = 5,
    REG_STATUS_EEWA         = 1 << 3,
    REG_EEPROM0             = 6,
};

// static const i2c_config *i2c;

static const i2c_channel mcp4661_i2c = {
    .i_base_address = I2C1,
    .i_is_master    = true,
    .i_stop         = true,
    .i_address      = 0b01010000,
};

static const uint16_t vol_steps[] = {
    // From MCP4XXX datasheet, Table 8-1
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 13, 14, 16, 18, 20, 23, 26, 29,
    32, 36, 41, 46, 51, 57, 64, 72, 81, 91,
    102, 114, 128, 144, 162, 181, 203, 228, 256
};
static const size_t vol_step_count = (&vol_steps)[1] - vol_steps;

static uint16_t saved_raw_vol;

static uint16_t mcp4661_read_register(uint8_t reg)
{
    uint8_t out[2] = { reg << 4 | 0x0C };
    uint8_t in[2] = { 0xFF, 0xFF };
    i2c_transmit(&mcp4661_i2c, out, 2);
    i2c_receive(&mcp4661_i2c, in, 2);
    return in[0] << 8 | in[1];
}

static void mcp4661_write_register(uint8_t reg, uint16_t value)
{
    uint8_t out[2] = { reg << 4 | value >> 8, value & 0xFF };
    i2c_transmit(&mcp4661_i2c, out, 2);
}

static void mcp4661_write_volatile_wipers(uint16_t value0, uint16_t value1)
{
    uint8_t buf[4] = {
        REG_VOLATILE_WIPER_0 << 4 | value0 >> 8, value0 & 0xFF,
        REG_VOLATILE_WIPER_1 << 4 | value1 >> 8, value1 & 0xFF,
    };
    i2c_transmit(&mcp4661_i2c, buf, 4);
}

static void mcp4661_write_nonvolatile_wipers(uint16_t value0, uint16_t value1)
{
    while (mcp4661_read_register(REG_STATUS) & REG_STATUS_EEWA)
        continue;
    mcp4661_write_register(REG_NONVOLATILE_WIPER_0, value0);
    // I don't know why, but it locks up if we don't delay here.
    delay_msec(5);
    while (mcp4661_read_register(REG_STATUS) & REG_STATUS_EEWA)
        continue;
    mcp4661_write_register(REG_NONVOLATILE_WIPER_1, value1);
    delay_msec(5);
}

void volume_init(void)
{
    assert(VOLUME_MAX + 1 == vol_step_count);
    saved_raw_vol = mcp4661_read_register(REG_VOLATILE_WIPER_0);
}

uint8_t volume_get(void)
{
    uint16_t raw_vol = volume_get_raw();
    size_t lo, hi;
    for (lo = 0, hi = vol_step_count; lo < hi; ) {
        size_t mid = (lo + hi) / 2;
        uint16_t mid_vol = vol_steps[mid];
        if (raw_vol == mid_vol) {
            return mid;
        }
        else if (raw_vol < mid_vol) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return lo < vol_step_count ? lo : vol_step_count; // close enough
}

void volume_set(uint8_t vol)
{
    assert(0 <= vol && vol <= VOLUME_MAX);
    volume_set_raw(vol_steps[vol]);
}

uint16_t volume_get_raw(void)
{
    return saved_raw_vol;
}

void volume_set_raw(uint16_t raw_vol)
{
    mcp4661_write_volatile_wipers(raw_vol, raw_vol);
    mcp4661_write_nonvolatile_wipers(raw_vol, raw_vol);
    saved_raw_vol = raw_vol;
}

#endif /* !AUDIO_REPAIR */
