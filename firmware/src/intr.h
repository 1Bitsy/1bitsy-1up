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

#ifndef INTR_H
#define INTR_H

#include <libopencm3/cm3/cortex.h>

#define WITH_INTERRUPTS_MASKED                                          \
     for (bool wim_interrupts_are_masked = cm_is_masked_interrupts(),   \
               wim_first_time = (cm_disable_interrupts(), true);        \
          wim_first_time;                                               \
          wim_interrupts_are_masked ? (void)0 : cm_enable_interrupts(), \
          wim_first_time = false)

#endif /* INTR_H */
