#ifndef TEXT_included
#define TEXT_included

#include "gfx-types.h"
#include "gfx-pixslice.h"

extern void text_init(void);

extern int  text_draw_str16 (gfx_pixslice *,
                             const char *str,
                             int x, int y,
                             gfx_rgb565 color);


extern void text_draw_char16(gfx_pixslice *,
                             char,
                             int x, int y,
                             gfx_rgb565 color);

#endif /* !TEXT_included */
