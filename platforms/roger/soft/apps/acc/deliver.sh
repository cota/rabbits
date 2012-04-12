#!/bin/bash

set -e

DEST=/home/cota/src/rabbits/platforms/roger/soft/initrd/rootfs_dir/acc
INITRD_DIR=/home/cota/src/rabbits/platforms/roger/soft/initrd

make
./makeall.sh
cp -v load.sh code0 out.* $DEST
cd $INITRD_DIR
./make_initramfs.sh
cd -
