#!/bin/bash

set -e

JPEG=/home/cota/src/jpeg_acc/jpeg0
PAR=4

for CONF in chstone411 lena411 fire411; do
    cd $JPEG/sysc_sim
    make clean
    make -j$PAR conf IMG=$CONF
    make
    cd -
    make clean
    make -j$PAR
    cp jpeg  out.jpeg.$CONF
    cp code0 out.code0.$CONF
done
