#ifndef PAM8019_included
#define PAM8019_included

// Driver for PAM8019 audio and headphone amplifier by Diodes
// Incorporated, as found on the 1UP audio repair board.

#include <stdbool.h>
#include <stdint.h>

typedef enum pam8019_mode {
    PM_SHUTDOWN,
    PM_MUTED,
    PM_NORMAL,
    PM_OVERRIDE_HP,             // force output to headphones.
    PM_OVERRIDE_SPKR,           // force output to speakers
    PM_END,
    PM_START = PM_SHUTDOWN,
} pam8019_mode;

extern void         pam8019_init                   (void);

// N.B., You will get no souond until you set the mode
// to NORMAL or OVERRIDE_*.

extern pam8019_mode pam8019_get_mode             (void);
extern void         pam8019_set_mode             (pam8019_mode);

extern bool         pam8019_output_is_headphones (void);

extern uint16_t     pam8019_get_raw_volume       (void);

#endif /* !PAM8019_included */
