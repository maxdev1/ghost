#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


# Define build setup
SRC=src-read
OBJ=obj-read
ARTIFACT_NAME=read.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS=""

# Include application build tasks
. "../applications.sh"
