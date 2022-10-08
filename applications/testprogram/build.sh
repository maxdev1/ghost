#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Build configuration
SRC="src"
ARTIFACT_NAME="tester.bin"
CFLAGS="-std=c++11 -I$SRC"
LDFLAGS="-lvbedriver -lps2driver"

# Include application build tasks
. "../applications.sh"
