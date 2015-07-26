#!/bin/sh
. ../../ghost.sh

# Define build setup
SRC=src
OBJ=obj
ARTIFACT_NAME=ps2driver.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS=""

# Include application build tasks
. ../applications.sh
