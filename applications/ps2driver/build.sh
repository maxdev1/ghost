#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
ARTIFACT_NAME="ps2driver.bin"
LDFLAGS="-lps2"

# Include application build tasks
. "../applications.sh"
