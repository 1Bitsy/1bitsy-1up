#include "volume.h"

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
