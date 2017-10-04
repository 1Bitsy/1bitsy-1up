#ifndef TEXT_included
#define TEXT_included

#include "gfx-types.h"
#include "gfx-pixslice.h"

extern void text_init(void);

extern void text_draw_char16(gfx_pixslice *,
                             char,
                             int x, int y,
                             gfx_rgb565 color);

#endif /* !TEXT_included */
