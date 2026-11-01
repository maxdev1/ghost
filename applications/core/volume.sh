#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

SRC="src-volume"
OBJ="obj-volume"
ARTIFACT_NAME="volume.bin"
LDFLAGS="-lstorage"
CFLAGS_ADD="-O2 -Wall -Wextra -Werror"

mkdir -p "$OBJ"
. "../applications.sh"
