#!/bin/bash

set -e

DEST=/home/cota/src/rabbits/platforms/roger/soft/initrd/rootfs_dir/acc
INITRD_DIR=/home/cota/src/rabbits/platforms/roger/soft/initrd

make
./makeall.sh

for img in heart411 earth411 stars411; do
    for type in code0 jpeg; do
	mv out.$type.$img out.slow.$type.$img
    done
done

cp -v load.sh runall.sh runsoft.sh runacc.sh out.* n_cycles $DEST
cd $INITRD_DIR
./make_initramfs.sh
cd -
