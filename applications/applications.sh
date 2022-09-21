#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh" 2 &>/dev/null
. "$ROOT/applications/shared.sh"

# Requirements
requireVar "ARTIFACT_NAME"
requireTool changes

# Variables
TARGET=$@

with TARGET "all"
with SRC "src"
with OBJ "obj"

ARTIFACT_LOCAL=$OBJ/$ARTIFACT_NAME
ARTIFACT_LOCAL_STATIC=$OBJ/$ARTIFACT_NAME_STATIC
ARTIFACT_TARGET=$SYSROOT_APPLICATIONS/$ARTIFACT_NAME
ARTIFACT_TARGET_STATIC=$SYSROOT_APPLICATIONS/$ARTIFACT_NAME_STATIC

# Targets
target_headline $TARGET

target_clean() {
	echo "cleaning:"
	remove $ARTIFACT_LOCAL

	if [ "$MAKE_STATIC" == 1 ]; then
		remove $ARTIFACT_LOCAL_STATIC
	fi

	cleanDirectory $OBJ
	changes --clear
}

target_link() {
	echo "linking:"
	$CROSS_CXX -o $ARTIFACT_LOCAL $OBJ/*.o $LDFLAGS
	list $ARTIFACT_LOCAL

	if [ "$MAKE_STATIC" == 1 ]; then
		$CROSS_CXX -static -o $ARTIFACT_LOCAL_STATIC $OBJ/*.o $LDFLAGS
		list $ARTIFACT_LOCAL_STATIC
	fi
}

target_clean_target() {
	echo "cleaning target:"
	rm $ARTIFACT_TARGET 2 &>/dev/null
	list $ARTIFACT_TARGET

	if [ "$MAKE_STATIC" == 1 ]; then
		rm $ARTIFACT_TARGET_STATIC 2 &>/dev/null
		list $ARTIFACT_TARGET_STATIC
	fi
}

target_install() {
	target_clean_target

	echo "installing artifact"
	cp $ARTIFACT_LOCAL $ARTIFACT_TARGET

	if [ "$MAKE_STATIC" == 1 ]; then
		echo "installing static artifact"
		cp $ARTIFACT_LOCAL_STATIC $ARTIFACT_TARGET_STATIC
	fi
}

print_help() {
	echo "Usage: $0"
	echo "  clean         cleans the output directory"
	echo "  all           builds, links and installs the application"
	echo ""
}

# Execute
for var in $TARGET; do
	if [[ "$var" = "all" ]]; then
		target_compile
		target_link
		target_install

	elif [[ $var == "clean" ]]; then
		target_clean

	elif [[ "$var" = "--help" || "$var" = "-h" || "$var" = "?" ]]; then
		print_help
		exit 0

	else
		echo "unknown target: '$var'"
		exit 1
	fi
done

target_successful
exit 0
