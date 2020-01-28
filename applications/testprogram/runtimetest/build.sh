#!/bin/bash
ROOT="../../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


# Define build setup
SRC=src
OBJ=obj
ARTIFACT_NAME=runtimetest.bin
ARTIFACT_NAME_STATIC=runtimetest-static.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS="-lruntimetest -shared-libgcc"
MAKE_STATIC=1

# Include application build tasks
. "../../applications.sh"
