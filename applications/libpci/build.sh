#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Define build setup
ARTIFACT_NAME="libpci.a"
ARTIFACT_NAME_SHARED="libpci.so"

# Include application build tasks
. "../libraries.sh"
