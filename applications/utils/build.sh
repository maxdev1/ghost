#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

$SH echo.sh $1
$SH ls.sh $1
$SH reverse.sh $1
$SH lines.sh $1
$SH read.sh $1
$SH write.sh $1
