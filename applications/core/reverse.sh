#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


# Define build setup
SRC="src-reverse"
OBJ="obj-reverse"
ARTIFACT_NAME="reverse.bin"

# Include application build tasks
. "../applications.sh"
