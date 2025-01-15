#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


# Define build setup
SRC="src-ls"
OBJ="obj-ls"
ARTIFACT_NAME="ls.bin"

# Include application build tasks
. "../applications.sh"
