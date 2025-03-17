#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
ARTIFACT_NAME="vmsvgadriver.bin"
LDFLAGS="-lpci -ldevice"

# Include application build tasks
. "../applications.sh"
