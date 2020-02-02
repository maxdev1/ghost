#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


# Define build setup
SRC=src
OBJ=obj
ARTIFACT_NAME=tester.bin
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS="-lghostuser -lcairo -lfreetype -lpixman-1 -lpng -lz"


# Build runtime test library
pushd libruntimetest
bash ./build.sh $1
popd

# Build runtime test binary
pushd runtimetest
bash ./build.sh $1
popd

# Include application build tasks
. "../applications.sh"
