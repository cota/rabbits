#!/bin/bash

HERE=`pwd`

if [ -z "${RABBITS_DIR}" ]; then
	echo "You should source the rabbits_env"
	exit
fi

if [ -z "${RABBITS_CROSS_COMPILE}" ]; then 
	echo "You should source the soft_env"
	exit
fi

ln -fs ${RABBITS_DIR}/rabbits/components/qemu_wrapper/qemu_wrapper_cts.h qemu_wrapper_cts.h

export KERNEL_VERSION=2.6.32-rc3
export KDIR=${HERE}/../../linux
export PATH=${PATH}:${RABBITS_XTOOLS}/bin

make || exit

mkdir -p ${HERE}/../../../initrd/rootfs_dir/lib/modules/${KERNEL_VERSION}
cp rabbitsfb.ko ${HERE}/../../../initrd/rootfs_dir/lib/modules/${KERNEL_VERSION}

cd ${HERE}/../../../initrd && ./make_initramfs.sh
