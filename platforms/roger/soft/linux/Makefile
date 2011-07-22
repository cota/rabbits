# Make sure ARCH and CROSS_COMPILE are properly set in your environment

KVER ?= 3.0

KDIR := linux-$(KVER)
CONFIG := $(KDIR)/.config

all: kernel drivers

kernel: $(KDIR) $(CONFIG)
	$(MAKE) -C $(KDIR)

drivers:
	$(MAKE) -C $@ KVER=$(KVER)

$(CONFIG):
	cp configs/config $@
	$(MAKE) -C $(KDIR) oldnoconfig

.PHONY: all kernel drivers
