#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
ARTIFACT_NAME="windowserver.bin"
CFLAGS_ADD="-I$SYSROOT_SYSTEM_INCLUDE/freetype2 -msse2"
LDFLAGS="-linput -lvbedriver -lvmsvgadriver -lps2driver -lproperties -lwindow -lfont -lcairo -lfreetype -lpixman-1 -lpng -lz"

# Include application build tasks
. "../applications.sh"
