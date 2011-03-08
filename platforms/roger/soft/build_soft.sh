#! /bin/bash
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

print_step(){
    echo "==================================="
    echo -en "\033[01;32m"
    echo " "$1
    echo -en "\033[00m"
    echo "==================================="
}

print_substep(){

    echo "++++++++"
    echo " "$1
    echo "++++++++"
}

print_error(){
    echo -en "\033[01;31m"
    echo "!---------------------------------!"
    echo " "$1
    echo "!---------------------------------!"
    echo -en " \033[00m"
}

sanity_checks(){

	GIT=`which git`

	if [ -z "${GIT}" ]; then 
		echo "You need to install git (version above 1.6.8)"
		exit
	fi

	GIT_VER=`${GIT} --version | cut -f3 -d' '`

	case ${GIT_VER} in

		1.[1-5].* | 1.6.[1-7].*)
			echo "Your version of git is too old"
			echo " Recommended version: above 1.6.8"
			echo " your version: ${GIT_VER}"
			exit
			;;
		1.6.[8-9].* | 1.[7-9].*)
			# Version OK
			#echo "git version OK"
			;;
		*)
			# sink point
			echo "Error in git version check"
			exit
			;;
	esac
}

linux_install(){

    print_step "Installing and compiling Linux ..."
    
    cd ${HERE}/linux/scripts
    ./install-linux-git.sh  || return
    ./compile_linux.sh      || return

    touch ${STAMPS_DIR}/linux_installed
}

initrd_install(){

    print_step "Creating InitRD ..."

    cd ${HERE}/initrd/scripts
    ./create_initial_rootfs.sh || return

    cd ${HERE}/busybox/scripts
    ./install.sh               || return

    cd ${HERE}/initrd
    ./make_initramfs.sh || return

    touch ${STAMPS_DIR}/initrd_installed
}

drivers_install(){

	print_step "Installing Drivers ..."

	cd ${HERE}/linux/drivers/scripts
	./install_drivers.sh || return


	touch ${STAMPS_DIR}/drivers_installed
}

apps_install(){

    print_step "Installing Apps ..."

	cd ${HERE}/apps/scripts
	./install_mjpeg.sh || return

	cd ${HERE}/apps/scripts
	./install_h264.sh || return

    cd ${HERE}/apps/hac
    ./compile_arm.sh || return

    touch ${STAMPS_DIR}/apps_installed
}

install_error(){

    print_error "Error in installation ..."
    exit

}

# ===========
# Main things
# ===========

STAMPS_DIR=${HERE}/.stamps

mkdir -p ${STAMPS_DIR}

if [ -e ./soft_env ]; then
	source ./soft_env
else
	echo "You need to set the soft_env file using the soft_env.example"
	exit
fi

sanity_checks || install_error

[ -e ${STAMPS_DIR}/linux_installed ] || \
    linux_install || install_error

[ -e ${STAMPS_DIR}/initrd_installed ] || \
    initrd_install || install_error

[ -e ${STAMPS_DIR}/drivers_installed ] || \
    drivers_install || install_error

[ -e ${STAMPS_DIR}/apps_installed ] || \
    apps_install || install_error

print_step "Done"
echo "You will have to source the soft_env file"
echo "each time you want to compile any step"
