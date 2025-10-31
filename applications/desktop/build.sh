#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
ARTIFACT_NAME="desktop.bin"
CFLAGS_ADD="-I$SYSROOT_SYSTEM_INCLUDE/freetype2"
LDFLAGS="-lproperties -lwindow -lcairo -lfreetype -lpixman-1 -lpng -lz"

# Include application build tasks
. "../applications.sh"
