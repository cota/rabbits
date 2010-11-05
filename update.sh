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

rabbits_update(){

    print_step "Updating RABBITS ..."
    echo "Pulling git (sls_repository)"
	git pull -q origin master
}

qemu_update(){

    print_step "Updating QEMU sim ..."
    
    cd ${HERE}/rabbits/qemu/scripts
    ./install.sh  || return
}

tools_update(){

    print_step "Updating Tools ..."

    cd ${HERE}/rabbits/tools
    make || return
}

install_error(){

    print_error "Error in installation ..."
    exit

}

# Main

if [ -e ./rabbits_env ]; then
	source ./rabbits_env
else
	echo "You need to set the rabbits_env file using the rabbits_env.example"
	exit
fi

rabbits_update || install_error
qemu_update    || install_error
tools_update   || install_error

print_step "Done"
