#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Define build setup
ARTIFACT_NAME="libwindow.a"
ARTIFACT_NAME_SHARED="libwindow.so"
CFLAGS="-std=c++11"
LDFLAGS="-shared"

# Include application build tasks
. "../libraries.sh"
