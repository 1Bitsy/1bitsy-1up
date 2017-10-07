#ifndef VOLUME_included
#define VOLUME_included

#include <stdint.h>

#define VOLUME_MIN 0
#define VOLUME_MAX 38
#define VOLUME_RAW_MAX 256

extern void     volume_init    (void);
extern uint8_t  volume_get     (void);
extern void     volume_set     (uint8_t);

extern uint16_t volume_get_raw (void);
extern void     volume_set_raw (uint16_t);

#endif /* !VOLUME_included */
