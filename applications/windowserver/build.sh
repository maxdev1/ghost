#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
SRC="src"
ARTIFACT_NAME="windowserver.bin"
CFLAGS="-std=c++11 -I$SYSROOT_SYSTEM_INCLUDE/freetype2 -I$SRC"
LDFLAGS="-linput -lvbedriver -lps2driver -lproperties -lwindow -lfont -lcairo -lfreetype -lpixman-1 -lpng -lz"

# Include application build tasks
. "../applications.sh"
