##
## Adapted from:
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

# although GNU makes doc say '::' rules are executed in order, this is only
# true for order of the lines. Apparently, targets on a single line might be
# executed not in order.

TOP:: wlan_keys.h 
TOP:: opencm3
TOP:: cc3klib 
TOP:: all

wlan_keys.h: wlan_keys_template.h
	test -e wlan_keys.h || cp $< $@

allclean: clean
	cd cc3000-simple/src; make clean
	rm -f *~ .*~ *.o
	
opencm3:
	@echo Building modified libopencm3
	cd libopencm3 ; make 

cc3klib: 
	@echo Building CC3000 library
	cd cc3000-simple/src; CC='$(CC)' CFLAGS='$(CFLAGS)' make 

BINARY = echoserver

OBJS = lowlevel.o stlinky.o 


TOOLCHAIN_DIR=libopencm3/

LDSCRIPT = ./stm32f0-discovery.ld
#LDFLAGS = --specs=rdimon.specs -lrdimon
LDFLAGS   = ./cc3000-simple/src/libcc3000.a
CFLAGS   = -Icc3000-simple/include/

include Makefile.include


