#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


# Define build setup
SRC="src-write"
OBJ="obj-write"
ARTIFACT_NAME="write.bin"

# Include application build tasks
. "../applications.sh"
