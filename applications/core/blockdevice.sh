#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

SRC="src-blockdevice"
OBJ="obj-blockdevice"
ARTIFACT_NAME="blockdevice.bin"
LDFLAGS="-lblockdevice"
CFLAGS_ADD="-O2 -Wall -Wextra -Werror"

mkdir -p "$OBJ"
. "../applications.sh"
