#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

SRC="src-filesystem"
OBJ="obj-folder"
ARTIFACT_NAME="folder.bin"
CFLAGS_ADD="-O2 -DFILESYSTEM_TOOL_FOLDER=1"

mkdir -p "$OBJ"
. "../applications.sh"
