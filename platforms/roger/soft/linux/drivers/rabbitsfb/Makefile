

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

