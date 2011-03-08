#! /bin/bash

HERE=${PWD}

export CROSS_COMPILE=${RABBITS_CROSS_COMPILE}
export PATH=${PATH}:${RABBITS_XTOOLS}/bin

CC=${CROSS_COMPILE}gcc \
	make || exit

cp hac ../../initrd/rootfs_dir/
cd ../../initrd && ./make_initramfs.sh

