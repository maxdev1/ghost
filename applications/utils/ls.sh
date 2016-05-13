#!/bin/sh
. ../../ghost.sh

# Define build setup
SRC=src-ls
OBJ=obj-ls
ARTIFACT_NAME=ls.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS=""

# Include application build tasks
. ../applications.sh
