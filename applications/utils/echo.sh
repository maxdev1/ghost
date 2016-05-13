#!/bin/sh
. ../../ghost.sh

# Define build setup
SRC=src-echo
OBJ=obj-echo
ARTIFACT_NAME=echo.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS=""

# Include application build tasks
. ../applications.sh
