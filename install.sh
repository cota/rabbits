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

[ -e ${STAMPS_DIR}/qemu_installed ] || \
    qemu_install || install_error

[ -e ${STAMPS_DIR}/tools_installed ] || \
    tools_install || install_error

print_step "Done"
