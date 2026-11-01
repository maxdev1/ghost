#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

SRC="src-disk"
OBJ="obj-disk"
ARTIFACT_NAME="disk.bin"
LDFLAGS="-lstorage"
CFLAGS_ADD="-O2 -Wall -Wextra -Werror"

mkdir -p "$OBJ"
. "../applications.sh"
