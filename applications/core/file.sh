#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

SRC="src-filesystem"
OBJ="obj-file"
ARTIFACT_NAME="file.bin"
CFLAGS_ADD="-O2 -DFILESYSTEM_TOOL_FOLDER=0"

mkdir -p "$OBJ"
. "../applications.sh"
