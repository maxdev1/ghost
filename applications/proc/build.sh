#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
SRC="src"
ARTIFACT_NAME="proc.bin"
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS=""

# Include application build tasks
. "../applications.sh"
