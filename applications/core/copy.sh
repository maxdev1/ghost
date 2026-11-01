#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

SRC="src-copy"
OBJ="obj-copy"
ARTIFACT_NAME="copy.bin"
CFLAGS_ADD="-O2"

. "../applications.sh"
