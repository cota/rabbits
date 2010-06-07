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

QEMU_DIR=qemu-0.9.1
QEMU_BRANCH=0.9.1-rabbits
SLS_REPO=git://tima-sls.imag.fr/QEmu.git

cd ${HERE}/..
mkdir -p ${LOG_DIR}

if [ -e ${QEMU_DIR} ]; then
echo "Pulling git (sls_repository)"
cd ${QEMU_DIR}
git pull -q origin ${QEMU_BRANCH}
else
echo "Cloning git (sls_repository)"
git clone -q ${SLS_REPO} -b ${QEMU_BRANCH} ${QEMU_DIR}
cd ${QEMU_DIR}
fi

echo "Configuring Qemu ..."
./configure &> ${LOG_DIR}/config.log   || exit

echo "Compiling and installing Qemu ..."
make &> ${LOG_DIR}/make.log          || exit
