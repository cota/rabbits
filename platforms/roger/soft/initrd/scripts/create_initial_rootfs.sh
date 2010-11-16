#!/bin/bash
#
# Copyright (c) 2010 TIMA Laboratory
#
# This file is part of Rabbits.
#
# Rabbits is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Rabbits is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Rabbits.  If not, see <http://www.gnu.org/licenses/>.
#

HERE=`pwd`

ROOTFS_DIR=${HERE}/../rootfs_dir
BASE_FILES_DIR=${HERE}/../base_files

XTOOLS_DIR=${RABBITS_XTOOLS}
CROSS_COMPILE=${RABBITS_CROSS_COMPILE}

ROOT_DIRS="/bin /dev /etc /lib /proc /sbin /sys /tmp /usr /usr/bin /usr/sbin /var"

echo "Creating basic rootfs ..."
mkdir -p ${ROOTFS_DIR}
cd ${ROOTFS_DIR}

for i in ${ROOT_DIRS}; do
    mkdir -p ./$i
#    echo Creating .$i
done

echo "Populating the rootfs ..."

BASE_FILES="/init /etc/inittab /etc/fstab"

for i in ${BASE_FILES}; do
    cp ${BASE_FILES_DIR}/`basename $i` ./$i
done

echo "Copying libraries ..."

#cp -r ${XTOOLS_DIR}/`basename ${CROSS_COMPILE} -`/sys-root/* ${ROOTFS_DIR} 
cp ${XTOOLS_DIR}/`basename ${CROSS_COMPILE} -`/libc/lib/ld-${RABBITS_LIBC_VERSION}.so ${ROOTFS_DIR}/lib/ld-linux.so.3
cp ${XTOOLS_DIR}/`basename ${CROSS_COMPILE} -`/libc/lib/libc-${RABBITS_LIBC_VERSION}.so ${ROOTFS_DIR}/lib/libc.so.6
cp ${XTOOLS_DIR}/`basename ${CROSS_COMPILE} -`/libc/lib/libgcc_s.so.1 ${ROOTFS_DIR}/lib/libgcc_s.so.1
cp ${XTOOLS_DIR}/`basename ${CROSS_COMPILE} -`/libc/lib/libgcc_s.so.1 ${ROOTFS_DIR}/lib/libgcc_s.so.1
cp ${XTOOLS_DIR}/`basename ${CROSS_COMPILE} -`/libc/lib/libm.so.6 ${ROOTFS_DIR}/lib/libm.so.6
cp ${XTOOLS_DIR}/`basename ${CROSS_COMPILE} -`/libc/lib/libpthread.so.0 ${ROOTFS_DIR}/lib/libpthread.so.0
