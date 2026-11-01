#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"
ARTIFACT_NAME="libstorage.a"
ARTIFACT_NAME_SHARED="libstorage.so"
CFLAGS_ADD="-O2 -fPIC -Wall -Wextra -Werror"
mkdir -p obj
. "../libraries.sh"
