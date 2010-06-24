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

LINUX_VER=2.6.31

HERE=`pwd`

LOG_DIR=${HERE}/../logs

if [ "x${RABBITS_XTOOLS}" == "x" ]; then 
	echo "You should install a cross-toolchain"
	echo "and set RABBITS_XTOOLS to the corresponding directory"
	exit
fi

mkdir -p ${LOG_DIR}
cd ${HERE}/../linux

export PATH=${RABBITS_XTOOLS}/bin:${PATH}
export ARCH=arm
export CROSS_COMPILE=${RABBITS_CROSS_COMPILE}

echo "Compiling Linux ..."

make zImage &> ${LOG_DIR}/compile.log || exit
