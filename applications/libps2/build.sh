#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Define build setup
ARTIFACT_NAME="libps2.a"
ARTIFACT_NAME_SHARED="libps2.so"
CFLAGS_ADD="-fPIC"

# Include application build tasks
. "../libraries.sh"
