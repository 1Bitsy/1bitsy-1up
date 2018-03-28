#ifndef VOLUME_included
#define VOLUME_included

#include <stdint.h>

#define VOLUME_MIN        0
#define VOLUME_MAX       64
#define VOLUME_RAW_MAX 4095

extern void     volume_init    (void);
extern uint8_t  volume_get     (void);

extern uint16_t volume_get_raw (void);

#endif /* !VOLUME_included */
