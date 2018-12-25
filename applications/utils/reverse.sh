#!/bin/bash
source ../../ghost.sh

# Define build setup
SRC=src-reverse
OBJ=obj-reverse
ARTIFACT_NAME=reverse.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS=""

# Include application build tasks
source ../applications.sh
