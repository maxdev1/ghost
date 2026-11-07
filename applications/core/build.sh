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
$SH copy.sh $@
failOnError
$SH file.sh $@
failOnError
$SH folder.sh $@
failOnError
$SH topic.sh $@
$SH blockdevice.sh $@
failOnError
$SH device.sh $@
$SH volume.sh $@
failOnError
$SH disk.sh $@
failOnError
$SH process.sh $@
failOnError
rm -f "$SYSROOT_APPLICATIONS/proc.bin"
