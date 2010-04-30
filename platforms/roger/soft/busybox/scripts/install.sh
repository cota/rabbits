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

#!/bin/bash

HERE=`pwd`

BUSYBOX_VER=1.12.1

BUSYBOX_REP=http://busybox.net/downloads
BUSYBOX_ARCHIVE=busybox-${BUSYBOX_VER}.tar.bz2
BUSYBOX_DIR=busybox-${BUSYBOX_VER}

CONFIGS_DIR=${HERE}/../configs
CONFIG_FILE=config-${BUSYBOX_VER}-roger

TOOLCHAIN_PATH=${RABBITS_XTOOLS}/bin
CROSS_COMPILE=${RABBITS_CROSS_COMPILE}

RFS_DIR=${HERE}/../../initrd/rootfs_dir

TMP_FILE=/tmp/busybox_config
LOG_DIR=${HERE}/../logs


echo "Getting Busybox Archive ..."

cd ${HERE}/..
mkdir -p ${LOG_DIR}

wget -Nq ${BUSYBOX_REP}/${BUSYBOX_ARCHIVE} || exit

echo "Uncompressing Busybox archive ..."
tar jxf ${BUSYBOX_ARCHIVE} || exit


cd ${HERE}/../${BUSYBOX_DIR}

echo "Configuring Busybox ..."
sed -e s#__COMPILER_LOC__#${TOOLCHAIN_PATH}/${CROSS_COMPILE}#g \
    -e s#__INSTALL_DIR__#${RFS_DIR}#g                          \
    ${CONFIGS_DIR}/${CONFIG_FILE} > .config || exit


echo "Compiling and Installing busybox ..."

make &> ${LOG_DIR}/compil.log          || exit
make install &> ${LOG_DIR}/install.log || exit

echo "Done."

