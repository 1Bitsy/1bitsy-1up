/*
 * This file is part of the 1Bitsy lcd graphics library.
 *
 * Copyright (C) 2017 Bob Miller <kbob@jogger-egg.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GFX_PIXTILE_H
#define GFX_PIXTILE_H

#include <unistd.h>

#include "gfx-types.h"

struct gfx_pixtile {
    gfx_rgb565 *pixels;         // N.B., points to (0, 0) which may not exist.
    int         x, y;           // origin
    size_t      w, h;           // size
    ssize_t     stride;         // row stride in pixels
};

extern void gfx_init_pixtile(gfx_pixtile *tile,
                             void *buffer,
                             int x, int y,
                             size_t w, size_t h,
                             size_t stride);
                         
static inline gfx_rgb565 *gfx_pixel_address_unchecked(gfx_pixtile *tile,
                                                      int x, int y)
{
    return tile->pixels + y * tile->stride + x;
}

extern gfx_rgb565 *gfx_pixel_address(gfx_pixtile *tile, int x, int y);

// Copy source pixmap into destination.
// offset translates src coordinates to dest.
// dest coord = src coord + offset.
extern void gfx_copy_pixtile(gfx_pixtile       *dest,
                             gfx_pixtile const *src,
                             gfx_ipoint         offset);

#endif /* GFX_PIXTILE_H */
