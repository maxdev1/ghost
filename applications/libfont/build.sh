#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Define build setup
ARTIFACT_NAME="libfont.a"
ARTIFACT_NAME_SHARED="libfont.so"
CFLAGS_ADD="-I$SYSROOT_SYSTEM_INCLUDE/freetype2"

# Include application build tasks
. "../libraries.sh"
