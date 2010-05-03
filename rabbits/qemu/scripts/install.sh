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

LOG_DIR=${HERE}/../logs

cd ${HERE}/..

git clone git://tima-sls.imag.fr/QEmu.git -b 0.9.1-rabbits qemu-0.9.1

mkdir -p ${LOG_DIR}

cd qemu-0.9.1

echo "Configuring Qemu ..."
./configure &> ${LOG_DIR}/config.log   || exit

echo "Compiling and installing Qemu ..."
make &> ${LOG_DIR}/make.log          || exit
