#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

TARGET=$@

with TARGET "all"

$SH echo.sh $@
$SH ls.sh $@
$SH reverse.sh $@
$SH lines.sh $@
$SH read.sh $@
$SH write.sh $@
