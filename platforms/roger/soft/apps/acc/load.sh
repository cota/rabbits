#/bin/sh

insmod uio
insmod uio_pdrv
mount -t sysfs none /sys
mount -t debugfs none /debug
mdev -s
