#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
ARTIFACT_NAME="tester.bin"
LDFLAGS="-lps2driver -lfenster"

# Include application build tasks
. "../applications.sh"
