KVER ?= 2.6.32-rc3
SUBDIRS := rabbitsfb h264dbf_drv qemu_drv

.PHONY: all install clean $(SUBDIRS)

all install clean: $(SUBDIRS)

all: TARGET=all
install: TARGET=install
clean: TARGET=clean

$(SUBDIRS): force
	$(MAKE) KVER=$(KVER) -C $@ $(TARGET)

.PHONY: force
force :;
