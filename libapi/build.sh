#!/bin/sh
. ../ghost.sh

TARGET=$1

with TARGET					"all"
with SRC					"src"
with OBJ					"obj"
with INC					"inc"
with INC_KERNEL				"../kernel/inc"

with ARTIFACT_NAME			"libghostapi.a"
ARTIFACT_LOCAL="$ARTIFACT_NAME"
ARTIFACT_TARGET="$SYSROOT_SYSTEM_LIB/$ARTIFACT_NAME"

with CFLAGS					"-std=c++11 -I$INC -I$INC_KERNEL"


echo "target: $TARGET"
requireTool changes

# always create output folder
mkdir -p $OBJ


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
	$CROSS_AR -r $ARTIFACT_LOCAL $OBJ/*.o
}
	
target_clean_target() {
	
	echo "removing $ARTIFACT_TARGET"
	rm $ARTIFACT_TARGET 2&> /dev/null
}

target_install_headers() {

	echo "installing api headers"
	cp -r $INC/* $SYSROOT_SYSTEM_INCLUDE/

	echo "installing kernel headers"
	cp -r $INC_KERNEL/* $SYSROOT_SYSTEM_INCLUDE/
	
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
if [[ $TARGET == "install-headers" ]]; then
	target_install_headers

elif [[ $TARGET == "all" ]]; then
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