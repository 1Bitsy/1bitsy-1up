#ifndef VOLUME_included
#define VOLUME_included

#include <stdint.h>

#define VOLUME_MIN 0
#define VOLUME_MAX 256

extern void volume_init(void);
extern uint16_t volume_get(void);
extern void volume_set(uint16_t);

#endif /* !VOLUME_included */
