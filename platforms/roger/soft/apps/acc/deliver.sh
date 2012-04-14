#!/bin/bash

set -e

DEST=/home/cota/src/rabbits/platforms/roger/soft/initrd/rootfs_dir/acc
INITRD_DIR=/home/cota/src/rabbits/platforms/roger/soft/initrd

make
./makeall.sh
cp -v load.sh runall.sh runsoft.sh runacc.sh out.* $DEST
cd $INITRD_DIR
./make_initramfs.sh
cd -
