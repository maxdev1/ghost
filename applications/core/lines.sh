#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


# Define build setup
SRC="src-lines"
OBJ="obj-lines"
ARTIFACT_NAME="lines.bin"

# Include application build tasks
. "../applications.sh"
