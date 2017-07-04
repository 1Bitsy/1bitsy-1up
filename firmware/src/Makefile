##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2017 Piotr Esden-Tempski <piotr@esden.net>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

BINARY = 1up-native

OBJS = main.o systick.o lcd.o pixslice.o gpio.o

LDSCRIPT = 1bitsy-stm32f415rgt-gfx.ld

# You can enable size optimization for project deployment, but it can get
# in the way of debugging your code
#CFLAGS		+= -Os

# If your project is using a specific C standard you can set it by uncommenting
# the following line and adjusting the -std flag accordingly
#CFLAGS		+= -std=gnu11 -pedantic

include ../libopencm3.target.mk

