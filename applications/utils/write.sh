#!/bin/sh
. ../../ghost.sh

# Define build setup
SRC=src-write
OBJ=obj-write
ARTIFACT_NAME=write.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS=""

# Include application build tasks
. ../applications.sh
