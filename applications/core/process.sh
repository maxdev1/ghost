#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

SRC="src-process"
OBJ="obj-process"
ARTIFACT_NAME="process.bin"

mkdir -p "$OBJ"
. "../applications.sh"
