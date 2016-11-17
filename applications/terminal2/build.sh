#!/bin/sh
. ../../ghost.sh

# Define build setup
SRC=src
OBJ=obj
ARTIFACT_NAME=terminal2.bin
CFLAGS="-std=c++11 -I$SRC -I$SYSROOT/system/include/freetype2"
LDFLAGS="-lcairo -lfreetype -lpixman-1 -lpng -lz"

# Include application build tasks
. ../applications.sh
