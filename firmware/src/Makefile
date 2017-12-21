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

OBJS := main.o audio.o audio_app.o button_boot.o gamepad.o		\
        gfx-pixslice.o gpio.o i2c.o lcd.o led.o munch_app.o systick.o	\
        text.o tile_app.o volume.o touch.o pam8019.o fblocks_app.o

LDSCRIPT = 1bitsy-stm32f415rgt-gfx.ld

V = 1
include ../libopencm3.target.mk

# You can enable size optimization for project deployment, but it can get
# in the way of debugging your code
cflags := $(filter_out -O%, $(CFLAGS)) -Werror -g
# In theory this -O should be valid for everything but video.o but somehow it
# affects everything? :(
OPT 	:= -O3
CFLAGS	 = $(cflags) $(OPT)

# If your project is using a specific C standard you can set it by uncommenting
# the following line and adjusting the -std flag accordingly
#CFLAGS		+= -std=gnu11 -pedantic

LDLIBS += -lm

lcd.o: OPT := -O0

text.o tile_app.o: assets/assets.h

assets/assets.h: assets/gen-assets.py assets/*.png assets/*.tmx assets/*/*.bdf
	@printf "  ASSETS  $?\n"
	$(Q)cd assets; python3 ./gen-assets.py > assets.h.new
	$(Q)mv assets/assets.h.new assets/assets.h

clean::
	$(Q)$(RM) -f assets/assets.h assets/assets.h.new


