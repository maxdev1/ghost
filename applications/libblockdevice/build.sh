#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

ARTIFACT_NAME="libblockdevice.a"
ARTIFACT_NAME_SHARED="libblockdevice.so"
CFLAGS_ADD="-O2 -fPIC"

. "../libraries.sh"
