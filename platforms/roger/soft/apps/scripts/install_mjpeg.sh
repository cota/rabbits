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

MJPEG_DIR=mjpeg
MJPEG_REP=git://tima-sls.imag.fr/Mjpeg.git

LOG_DIR=${HERE}/../${MJPEG_DIR}/logs

if [ "x${RABBITS_XTOOLS}" == "x" ]; then 
	echo "You should install a cross-toolchain"
	echo "and set RABBITS_XTOOLS to the corresponding directory"
	exit
fi

cd ..

if [ -d ${MJPEG_DIR}/.git/ ]; then
    (
	cd ${MJPEG_DIR}
	echo "Pulling mjpeg git repo (sls repository) ..."
	git pull -q ${MJPEG_REP} roger
    )
else
    rm -fr ${MJPEG_DIR}
    echo "Cloning Mjpeg git repo (sls repository) [can take a while] ..."
    git clone  ${MJPEG_REP} -b roger ${MJPEG_DIR}
fi

mkdir -p ${LOG_DIR}

cd ${MJPEG_DIR}

./compile_arm.sh &> ${LOG_DIR}/compile.log || exit