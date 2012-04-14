#!/bin/bash

set -e

JPEG=/home/cota/src/jpeg_acc/jpeg0
PAR=4

IMGS=$(for i in $JPEG/img/*.bmp; do (echo $i | sed 's|.*/\(.*\)\.bmp|\1|'); done | xargs)

for CONF in $IMGS; do
    cd $JPEG/sysc_sim
    make clean
    make -j$PAR conf IMG=$CONF
    make -j$PAR
    cd -
    make clean
    make -j$PAR
    cp jpeg  out.jpeg.$CONF
    cp code0 out.code0.$CONF
done
