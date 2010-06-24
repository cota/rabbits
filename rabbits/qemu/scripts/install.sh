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

failwith () {
    printf "$@"
    exit 1
}

HERE=`pwd`

LOG_DIR=${HERE}/../logs

QEMU_DIR=qemu-0.9.1
QEMU_BRANCH=0.9.1-rabbits
SLS_REPO=git://tima-sls.imag.fr/QEmu.git

cd ${HERE}/..
mkdir -p ${LOG_DIR}
rm -fr ../libs
mkdir -p ../libs

gv=`git --version|cut -d " " -f 3`
gv1=${gv%%\.*}
gv2=${gv#*\.}
gv2=${gv2%%\.*}

if [ $gv1 -lt 1 ] || [ $gv1 -eq 1 -a $gv2 -lt 7 ] ; then
    failwith "Your git version: $gv. Needed >= 1.7.0.4"
fi

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
./configure &> ${LOG_DIR}/config.log   || failwith "Configure QEmu failed. Please read the log for details:\n%s\n" "${LOG_DIR}/config.log"

echo "Compiling and installing Qemu ..."
make &> ${LOG_DIR}/make.log          || failwith "Compilation of QEmu failed. Please read the log for details:\n%s\n" "${LOG_DIR}/make.log"

