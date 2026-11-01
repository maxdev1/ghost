#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

ARTIFACT_NAME="libacpi.a"
ARTIFACT_NAME_SHARED="libacpi.so"
CFLAGS_ADD="-fPIC"

mkdir -p obj
. "../libraries.sh"
