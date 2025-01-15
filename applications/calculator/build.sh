#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
ARTIFACT_NAME="calculator.bin"
LDFLAGS="-lwindow"

# Include application build tasks
. "../applications.sh"
