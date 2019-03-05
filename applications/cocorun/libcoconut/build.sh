#!/bin/bash
ROOT="../../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

TARGET=$1

with TARGET					"all"
with SRC					"src"
with OBJ					"obj"
with INC					"inc"

with ARTIFACT_NAME			"libcoconut.so"
ARTIFACT_LOCAL="$ARTIFACT_NAME"
ARTIFACT_TARGET="$SYSROOT_SYSTEM_LIB/$ARTIFACT_NAME"

with CFLAGS					"-std=c++11 -fpic -I$INC -I$SRC"
with LDFLAGS				"-shared"


echo "target: $TARGET"
requireTool changes


target_clean() {
	echo "cleaning:"
	rm $ARTIFACT_LOCAL
	cleanDirectory $OBJ
	changes --clear
}

target_compile() {
	echo "compiling:"
	
	# check if headers have changed
	headers_have_changed=0
	for file in $(find "$INC" -iname "*.h" -o -iname "*.hpp"); do
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

target_archive() {
	echo "archiving:"
	$CROSS_CC $LDFLAGS -o $ARTIFACT_LOCAL $OBJ/*.o
}
	
target_clean_target() {
	
	echo "removing $ARTIFACT_TARGET"
	rm $ARTIFACT_TARGET 2&> /dev/null
}

target_install_headers() {

	echo "installing headers"
	cp -r $INC/* $SYSROOT_SYSTEM_INCLUDE/

	
}

target_install() {
	
	target_clean_target
	target_install_headers
	
	echo "installing artifact"
	cp $ARTIFACT_LOCAL $ARTIFACT_TARGET
	
	# c'mon
	chmod -R 777 $SYSROOT
}

# execute targets
if [[ $TARGET == "all" ]]; then
	target_compile
	target_archive
	target_install
	
elif [[ $TARGET == "clean" ]]; then
	target_clean
	
else
	echo "unknown target: '$TARGET'"
	exit 1
fi

exit 0