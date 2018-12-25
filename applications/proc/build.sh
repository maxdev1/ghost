#!/bin/bash
source ../../ghost.sh

# Define build setup
SRC=src
OBJ=obj
ARTIFACT_NAME=proc.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS=""

# Include application build tasks
source ../applications.sh
