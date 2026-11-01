#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

ARTIFACT_NAME="install.bin"
LDFLAGS="-lstorage -lfenster"
CFLAGS_ADD="-O2 -Wall -Wextra -Werror"
mkdir -p obj
. "../applications.sh"
