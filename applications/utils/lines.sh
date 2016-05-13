#!/bin/sh
. ../../ghost.sh

# Define build setup
SRC=src-lines
OBJ=obj-lines
ARTIFACT_NAME=lines.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS=""

# Include application build tasks
. ../applications.sh
