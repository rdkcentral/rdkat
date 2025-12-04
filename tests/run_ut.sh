#!/bin/sh
PARENT_DIR=$(dirname $(dirname $(realpath $0)))

if [ "$1" = "clean" ]; then
    TARGET=clean-test
else
    TARGET=test
fi

cd $PARENT_DIR && make -f Makefile.atspi2 $TARGET 
