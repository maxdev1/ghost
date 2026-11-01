#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"
ARTIFACT_NAME="storagemanager.bin"
LDFLAGS="-lblockdevice"
CFLAGS_ADD="-O2 -Wall -Wextra -Werror"
mkdir -p obj
. "../applications.sh"
