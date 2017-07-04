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

#ifndef GFX_TYPES_H
#define GFX_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef uint16_t gfx_rgb565;
typedef uint32_t gfx_rgb888;
typedef uint8_t  gfx_alpha8;

typedef struct gfx_pixslice gfx_pixslice;

typedef union gfx_point {
    struct {
        float x, y;             // use p.x, p.y
    };
    float c[2];                 // use p.c[i]
} gfx_point;

typedef union gfx_ipoint {
    struct {
        int x, y;               // use p.x, p.y
    };
    int c[2];                   // use p.c[i]
} gfx_ipoint;

typedef struct gfx_trapezoid {
    float xl0, xr0, y0;
    float xl1, xr1, y1;
} gfx_trapezoid;

typedef struct gfx_triangle {
    gfx_point v[3];
} gfx_triangle;

#endif /* GFX_TYPES_H */
