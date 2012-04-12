#/bin/sh

insmod uio
insmod uio_pdrv
mount -t sysfs none /sys
mdev -s
