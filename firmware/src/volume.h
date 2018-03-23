#ifndef VOLUME_included
#define VOLUME_included

#include <stdint.h>

#ifdef AUDIO_REPAIR
    #define VOLUME_MIN 0
    #define VOLUME_MAX 64
    #define VOLUME_RAW_MAX 4095
#else
    #define VOLUME_MIN 0
    #define VOLUME_MAX 38
    #define VOLUME_RAW_MAX 256
#endif

extern void     volume_init    (void);
extern uint8_t  volume_get     (void);
#ifndef AUDIO_REPAIR
    extern void volume_set     (uint8_t);
#endif

extern uint16_t volume_get_raw (void);
#ifndef AUDIO_REPAIR
    extern void volume_set_raw (uint16_t);
#endif

#endif /* !VOLUME_included */
