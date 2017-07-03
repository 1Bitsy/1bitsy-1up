##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
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

PREFIX		?= arm-none-eabi
#PREFIX		?= arm-elf

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
# Do not print "Entering directory ...".
MAKEFLAGS += --no-print-directory
endif

# In case you are using an older compiler or want to set the C standard to a
# specific version globally for the project this is where that would go.
#CFLAGS += -std=gnu11

OPENCM3_DIR := $(realpath libopencm3)
BUILD_RULES = elf

all: build

bin: BUILD_RULES += bin
hex: BUILD_RULES += hex
srec: BUILD_RULES += srec
list: BUILD_RULES += list
images: BUILD_RULES += images

bin: build
hex: build
srec: build
list: build
images: build

build: lib src

lib:
	$(Q)if [ ! "`ls -A libopencm3`" ] ; then \
		printf "######## ERROR ########\n"; \
		printf "\tlibopencm3 is not initialized.\n"; \
		printf "\tPlease run:\n"; \
		printf "\t$$ git submodule init\n"; \
		printf "\t$$ git submodule update\n"; \
		printf "\tbefore running make.\n"; \
		printf "######## ERROR ########\n"; \
		exit 1; \
		fi
	$(Q)$(MAKE) -C libopencm3 lib TARGETS="stm32/f4"

src: lib
	@printf "  BUILD   $@\n";
	$(Q)$(MAKE) --directory=$@ OPENCM3_DIR=$(OPENCM3_DIR) $(BUILD_RULES)

flash: src
	@printf "  FLASH   $@\n";
	$(Q)$(MAKE) --directory=src flash

clean:
	$(Q)$(MAKE) -C libopencm3 clean
	$(Q)$(MAKE) -C src clean

stylecheck: src.stylecheck
styleclean: src.styleclean


%.clean:
	$(Q)if [ -d $* ]; then \
		printf "  CLEAN   $*\n"; \
		$(MAKE) -C $* clean OPENCM3_DIR=$(OPENCM3_DIR) || exit $?; \
	fi;

%.styleclean:
	$(Q)$(MAKE) -C $* styleclean OPENCM3_DIR=$(OPENCM3_DIR)

%.stylecheck:
	$(Q)$(MAKE) -C $* stylecheck OPENCM3_DIR=$(OPENCM3_DIR)


.PHONY: build lib examples install clean stylecheck styleclean \
        bin hex srec list images

