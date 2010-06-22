#!/bin/bash

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

qemu_install(){

    print_step "Installing QEMU sim ..."
    
    cd ${HERE}/rabbits/qemu/scripts
    ./install.sh  || return

    touch ${STAMPS_DIR}/qemu_installed
}

tools_install(){

    print_step "Installing Tools ..."

    cd ${HERE}/rabbits/tools
    make || return

    touch ${STAMPS_DIR}/tools_installed
}

install_error(){

    print_error "Error in installation ..."
    exit

}


STAMPS_DIR=${HERE}/.stamps

source ./rabbits_env

mkdir -p ${STAMPS_DIR}

sanity_checks || install_error

[ -e ${STAMPS_DIR}/qemu_installed ] || \
    qemu_install || install_error

[ -e ${STAMPS_DIR}/tools_installed ] || \
    tools_install || install_error

print_step "Done"
