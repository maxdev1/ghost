#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

SRC="src-device"
OBJ="obj-device"
ARTIFACT_NAME="device.bin"
LDFLAGS="-ldevice"

mkdir -p "$OBJ"
. "../applications.sh"
