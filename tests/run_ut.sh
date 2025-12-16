#!/bin/sh

# Exit on error
set -e

PARENT_DIR=$(dirname $(dirname $(realpath $0)))

if [ "$1" = "clean" ]; then
    TARGET=clean-tests
else
    TARGET=run-tests
fi

cd $PARENT_DIR && SYSROOT_INCLUDES_DIR= make -f Makefile.atspi2 $TARGET 
