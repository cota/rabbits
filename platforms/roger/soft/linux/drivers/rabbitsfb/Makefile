#
# Copyright (c) 2010 TIMA Laboratory
#
# This file is part of Rabbits.
#
# Rabbits is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Rabbits is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Rabbits.  If not, see <http://www.gnu.org/licenses/>.
#

#KDIR 	         := /lib/modules/$(shell uname -r)/build
PWD              := $(shell pwd)
#EXTRA_CFLAGS     := -DDEBUG -DHDEBUG
EXTRA_CFLAGS     := 

obj-m            := rabbitsfb.o
rabbitsfb-objs   := rabbitsfb_drv.o rabbitsfb_int.o

.PHONY: all clean modules_install

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -f *.ko *.o .*.cmd .*.o.flags *.mod.c modules.order *.symvers *~

