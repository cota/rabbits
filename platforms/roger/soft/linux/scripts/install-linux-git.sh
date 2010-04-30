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

#if [ ! -e ${LINUX_ARCHIVE} ]; then
#    echo "Retrieving linux kernel archive ..." 
#    wget -q ${KERNEL_REP}${LINUX_ARCHIVE} || exit
#fi

#echo "Uncompressing the kernel ..."
#tar jxf ${LINUX_ARCHIVE} || exit

rm -fr ${LINUX_DIR}

git clone ${KERNEL_REP} -b roger ${LINUX_DIR}

#echo "Applying patches ..."
#if [ -e ${PATCH_DIR}/${LINUX_VER} ]; then
#    PATCHES=`ls ${PATCH_DIR}/${LINUX_VER}/*`
#fi
#if [ ! -z "${PATCHES}" ]; then
#    for i in ${PATCHES}; do
#	echo "Applying patch "$i
#	patch -p1 -d ${LINUX_DIR} < $i || exit
#    done
#else
#    echo "No patch applied"
#fi

echo "Configuring kernel ..."
cp ${CONFIGS_DIR}/${CONFIG_FILE} ${LINUX_DIR}/.config

echo "Now ready to be compiled !!!"