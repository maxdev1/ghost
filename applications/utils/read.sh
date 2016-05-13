#!/bin/sh
. ../../ghost.sh

# Define build setup
SRC=src-read
OBJ=obj-read
ARTIFACT_NAME=read.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS=""

# Include application build tasks
. ../applications.sh
