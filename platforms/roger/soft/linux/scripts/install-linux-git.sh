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

LINUX_DIR=linux
KERNEL_REP=git://tima-sls.imag.fr/Linux.git

CONFIGS_DIR=configs
CONFIG_FILE=config-${LINUX_VER}-roger

cd ..

if [ -d ${LINUX_DIR}/.git/ ]; then
    (
	cd ${LINUX_DIR}
	echo "Pulling Linux git repo (sls repository) ..."
	git pull -q ${KERNEL_REP} roger
    )
else
    rm -fr ${LINUX_DIR}
    echo "Cloning Linux git repo (sls repository) ..."
    git clone -q ${KERNEL_REP} -b roger ${LINUX_DIR}
fi

echo "Configuring kernel ..."
cp ${CONFIGS_DIR}/${CONFIG_FILE} ${LINUX_DIR}/.config
