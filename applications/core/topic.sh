#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

SRC="src-topic"
OBJ="obj-topic"
ARTIFACT_NAME="topic.bin"

mkdir -p "$OBJ"
. "../applications.sh"
