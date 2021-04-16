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
LDFLAGS="-lvbedriver -lps2"

# Include application build tasks
. "../applications.sh"
