KVER ?= 2.6.32-rc3
KDIR ?= ../../linux-$(KVER)
MDIR ?= ../../../initrd/rootfs_dir/lib/modules/$(KVER)

all: reqs
	$(MAKE) -C $(KDIR) M=`pwd`

install: all
	cp *.ko $(MDIR)
	cd ../../../initrd && ./make_initramfs.sh

clean:
	$(MAKE) -C $(KDIR) M=`pwd` clean

help:
	$(MAKE) -C $(KDIR) M=`pwd` help

.PHONY: all clean help reqs

reqs: $(RABBITS_DIR) $(MDIR)
	whereis $(RABBITS_CROSS_COMPILE)gcc

$(MDIR):
	mkdir -p $(MDIR)
