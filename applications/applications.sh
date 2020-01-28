#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh" 2&> /dev/null


TARGET=$1
with TARGET "all"

ARTIFACT_LOCAL=$OBJ/$ARTIFACT_NAME
ARTIFACT_LOCAL_STATIC=$OBJ/$ARTIFACT_NAME_STATIC
ARTIFACT_TARGET=$SYSROOT_APPLICATIONS/$ARTIFACT_NAME
ARTIFACT_TARGET_STATIC=$SYSROOT_APPLICATIONS/$ARTIFACT_NAME_STATIC

target_headline $TARGET
requireTool changes


target_clean() {
	echo "cleaning:"
	remove $ARTIFACT_LOCAL

	if [ "$MAKE_STATIC" == 1 ]; then
		remove $ARTIFACT_LOCAL_STATIC
	fi

	cleanDirectory $OBJ
	changes --clear
}

target_compile() {
	echo "compiling:"
	
	# check if headers have changed
	headers_have_changed=0
	for file in $(find "$SRC" -iname "*.h" -o -iname "*.hpp"); do
		changes -c $file
		if [ $? -eq 1 ]; then
			headers_have_changed=1
		fi
		changes -s $file
	done
	
	# compile sources
	for file in $(find "$SRC" -iname "*.c" -o -iname "*.cpp"); do 
		changes -c $file
		changed=$?
		if ( [ $headers_have_changed -eq 1 ] || [ $changed -eq 1 ] ); then
			out=`sourceToObject $file`
			list $out
			$CROSS_CXX -c $file -o "$OBJ/$out" $CFLAGS
			failOnError
			changes -s $file
		fi
	done
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
	rm $ARTIFACT_TARGET 2&> /dev/null
	list $ARTIFACT_TARGET

	if [ "$MAKE_STATIC" == 1 ]; then
		rm $ARTIFACT_TARGET_STATIC 2&> /dev/null
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


# execute targets
if [[ $TARGET == "all" ]]; then
	target_compile
	target_link
	target_install
	
elif [[ $TARGET == "clean" ]]; then
	target_clean
	
else
	echo "unknown target: '$TARGET'"
	exit 1
fi

target_successful
exit 0