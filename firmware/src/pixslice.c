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

#include "gfx-pixslice.h"

#include <string.h>

#include "math-util.h"

extern void gfx_init_pixslice(gfx_pixslice *slice,
                             void *buffer,
                             int x, int y,
                             size_t w, size_t h,
                             size_t stride)
{
    slice->pixels = (gfx_rgb565 *)buffer - y * stride - x;
    slice->x      = x;
    slice->y      = y;
    slice->w      = w;
    slice->h      = h;
    slice->stride = stride;
}

extern gfx_rgb565 *gfx_pixel_address(gfx_pixslice *slice, int x, int y)
{
    if (x < slice->x || x >= (ssize_t)(slice->x + slice->w))
        return NULL;
    if (y < slice->y || y >= (ssize_t)(slice->y + slice->h))
        return NULL;
    return gfx_pixel_address_unchecked(slice, x, y);
}

void gfx_copy_pixslice(gfx_pixslice       *dest,
                      gfx_pixslice const *src,
                      gfx_ipoint         offset)
{
    int x0s = MAX(src->x, dest->x - offset.x);
    int x1s = MIN(src->x + (int)src->w, dest->x + (int)dest->w - offset.x);
    if (x0s >= x1s)
        return;
    int x0d = x0s + offset.x;
    int nx = x1s - x0s;
    int y0s = MAX(src->y, dest->y - offset.y);
    int y1s = MIN(src->y + (int)src->h, dest->y + (int)dest->h - offset.y);
    if (y0s >= y1s)
        return;
    int y0d = y0s + offset.y;
    for (int ys = y0s, yd = y0d; ys < y1s; ys++, yd++) {
        const gfx_rgb565 *ps =
            gfx_pixel_address_unchecked((gfx_pixslice *)src, x0s, ys);
        gfx_rgb565 *pd = gfx_pixel_address_unchecked(dest, x0d, yd);
        memcpy(pd, ps, nx * sizeof *pd);
    }
}
