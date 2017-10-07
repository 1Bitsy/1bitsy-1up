#include "text.h"

#include "assets/assets.h"

void text_init(void)
{
}

int text_draw_str16(gfx_pixslice *slice,
                    const char *str,
                    int x, int y,
                    gfx_rgb565 color)
{
    while (*str) {
        text_draw_char16(slice, *str, x, y, color);
        x++;
        str++;
    }
    return x;
}

void text_draw_char16(gfx_pixslice *slice,
                      char ch,
                      int x, int y,
                      gfx_rgb565 color)
{
    int xoff = x * 8;
    int yoff = y * 16;

    if ((ch < MINIWI_FONT_OFFSET) ||
        (ch >= MINIWI_FONT_OFFSET + MINIWI_FONT_GLYPH_COUNT)) {
        return;
    }

    for (int py = 0; py < MINIWI_FONT_HEIGHT; py++) {
        gfx_rgb565 *px0 = gfx_pixel_address(slice, xoff, yoff + (py * 2));
        gfx_rgb565 *px1 = gfx_pixel_address(slice, xoff, yoff + (py * 2) + 1);
        for (int px = 0; px < MINIWI_FONT_WIDTH; px++) {
            if (miniwi_font[ch - MINIWI_FONT_OFFSET][px] & (1 << py)) {
                if (px0) {
                    px0[0] = color;
                    px0[1] = color;
                }
                if (px1) {
                    px1[0] = color;
                    px1[1] = color;
                }
            }
            if (px0) {
                px0 += 2;
            }
            if (px1) {
                px1 += 2;
            }
        }
    }
}
