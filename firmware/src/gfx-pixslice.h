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

#ifndef GFX_PIXSLICE_H
#define GFX_PIXSLICE_H

#include <unistd.h>

#include "gfx-types.h"

struct gfx_pixslice {
    gfx_rgb565 *pixels;         // N.B., points to (0, 0) which may not exist.
    int         x, y;           // origin
    size_t      w, h;           // size
    ssize_t     stride;         // row stride in pixels
};

void gfx_init_pixslice(gfx_pixslice *slice,
                             void *buffer,
                             int x, int y,
                             size_t w, size_t h,
                             size_t stride);
                         
static inline gfx_rgb565 *gfx_pixel_address_unchecked(gfx_pixslice *slice,
                                                      int x, int y)
{
    return slice->pixels + y * slice->stride + x;
}

gfx_rgb565 *gfx_pixel_address(gfx_pixslice *slice, int x, int y);

// Copy source pixmap into destination.
// offset translates src coordinates to dest.
// dest coord = src coord + offset.
void gfx_copy_pixslice(gfx_pixslice       *dest,
                             gfx_pixslice const *src,
                             gfx_ipoint         offset);

#endif /* GFX_PIXSLICE_H */
