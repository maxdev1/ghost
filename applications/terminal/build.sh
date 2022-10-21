#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
SRC="src"
ARTIFACT_NAME="terminal.bin"
CFLAGS="-std=c++11 -I$SYSROOT_SYSTEM_INCLUDE/freetype2 -I$SRC"
LDFLAGS="-lwindow -lfont -lterminal -lps2driver -linput -lproperties -lcairo -lfreetype -lpixman-1 -lpng -lz"

# Include application build tasks
. "../applications.sh"
