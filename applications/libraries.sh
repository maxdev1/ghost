#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"
. "$ROOT/applications/shared.sh"

#
# LIBRARY BUILD
#
# This is a preset build script for a library. It defines a set of targets to build the library,
# install it to the system target and also install the headers respectively.
#

# Requirements
requireVar "ARTIFACT_NAME"
requireVar "ARTIFACT_NAME_SHARED"
requireTool changes

# Variables
TARGET=$@

with TARGET "all"
with SRC "src"
with OBJ "obj"
with INC "inc"

with ARTIFACT_LOCAL "$ARTIFACT_NAME"
with ARTIFACT_TARGET "$SYSROOT_SYSTEM_LIB/$ARTIFACT_NAME"
with ARTIFACT_LOCAL_SHARED "$ARTIFACT_NAME_SHARED"
with ARTIFACT_TARGET_SHARED "$SYSROOT_SYSTEM_LIB/$ARTIFACT_NAME_SHARED"

with CFLAGS_ADD ""
with CFLAGS "-std=c++11 -I$INC $CFLAGS_ADD"
with LDFLAGS "-shared"

# Targets
target_headline $TARGET

target_clean() {
	echo "cleaning:"
	rm $ARTIFACT_LOCAL
	rm $ARTIFACT_LOCAL_SHARED
	cleanDirectory $OBJ
	changes --clear
}

target_archive() {
	echo "archiving:"
	$CROSS_AR -r $ARTIFACT_LOCAL $OBJ/*.o
	$CROSS_CC $LDFLAGS -o $ARTIFACT_LOCAL_SHARED $OBJ/*.o
}

target_clean_target() {
	echo "removing $ARTIFACT_TARGET"
	rm $ARTIFACT_TARGET 2 &>/dev/null
	echo "removing $ARTIFACT_TARGET_SHARED"
	rm $ARTIFACT_TARGET_SHARED 2 &>/dev/null
}

target_install_headers() {
	echo "installing headers"
	cp -r $INC/* $SYSROOT_SYSTEM_INCLUDE/
}

target_install() {
	target_clean_target
	target_install_headers

	echo "installing artifacts"
	cp $ARTIFACT_LOCAL $ARTIFACT_TARGET
	cp $ARTIFACT_LOCAL_SHARED $ARTIFACT_TARGET_SHARED
}

print_help() {
	echo "Usage: $0"
	echo "  clean             cleans the output directory"
	echo "  all               builds, links and installs the library"
	echo "  install-headers   installs only the headers"
	echo ""
}

# Execute
for var in $TARGET; do
	if [[ $var = "install-headers" ]]; then
		target_install_headers

	elif [[ $var == "all" ]]; then
		target_compile
		target_archive
		target_install

	elif [[ $var == "clean" ]]; then
		target_clean

	elif [[ "$var" = "--help" || "$var" = "-h" || "$var" = "?" ]]; then
		print_help
		exit 0

	else
		echo "unknown target: '$TARGET'"
		exit 1
	fi
done

exit 0
