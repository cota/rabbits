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

H264DEC_DIR=h264decoder
H264DEC_REP=git://tima-sls.imag.fr/H264Decoder.git
H264DEC_BRANCH=master

LOG_DIR=${HERE}/../${H264DEC_DIR}/logs

if [ "x${RABBITS_XTOOLS}" == "x" ]; then 
	echo "You should install a cross-toolchain"
	echo "and set RABBITS_XTOOLS to the corresponding directory"
	exit
fi

cd ..

if [ -d ${H264DEC_DIR}/.git/ ]; then
    (
	cd ${H264DEC_DIR}
	echo "Pulling h264 decoder git repo (sls repository) ..."
	git pull -q ${H264DEC_REP} ${H264DEC_BRANCH}
    )
else
    rm -fr ${H264DEC_DIR}
    echo "Cloning h264 decoder git repo (sls repository) [can take a while] ..."
    git clone -q ${H264DEC_REP} -b ${H264DEC_BRANCH} ${H264DEC_DIR}
fi

mkdir -p ${LOG_DIR}

cd ${H264DEC_DIR}
echo "Configuring H264 decoder ..."
./cfg &> ${LOG_DIR}/config.log || exit

echo "Installing H264 decoder ..."
./compile_arm.sh &> ${LOG_DIR}/compile.log || exit