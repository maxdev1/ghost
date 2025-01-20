#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
ARTIFACT_NAME="ahcidriver.bin"
LDFLAGS="-lpci"

# Include application build tasks
. "../applications.sh"
