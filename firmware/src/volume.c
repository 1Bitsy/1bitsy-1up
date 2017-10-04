#include "volume.h"

void volume_init(void)
{}

uint16_t volume_get(void)
{
#if 0
    static int x = VOLUME_MIN;
    if (++x > VOLUME_MAX)
        x = VOLUME_MIN;
#else
    #include "systick.h"
    uint32_t x = system_millis / 100 % VOLUME_MAX;
#endif
    return x;
}

void volume_set(uint16_t vol)
{}
