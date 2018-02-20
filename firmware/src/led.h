#ifndef LED_included
#define LED_included

// Control the LED on the 1Bitsy board.

#include <stdbool.h>

extern void led_init(void);

extern bool led_get(void);

extern void led_set(bool);
extern void led_on(void);
extern void led_off(void);
extern void led_toggle(void);

#endif /* ! LED_included */
