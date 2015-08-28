#!/bin/sh
. ../../ghost.sh

TARGET=$1
ARTIFACT_LOCAL=$OBJ/$ARTIFACT_NAME
ARTIFACT_TARGET=$SYSROOT_APPLICATIONS/$ARTIFACT_NAME

echo "target: $TARGET"
requireTool changes


target_clean() {
	echo "cleaning:"
	remove $ARTIFACT_LOCAL
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
	$CROSS_CXX -o $ARTIFACT_LOCAL $OBJ/*.o -lghostuser $LDFLAGS
	list $ARTIFACT_LOCAL
}
	
target_clean_target() {
	
	echo "cleaning target:"
	rm $ARTIFACT_TARGET 2&> /dev/null
	list $ARTIFACT_TARGET
}

target_install() {
	
	target_clean_target
	
	echo "installing artifact"
	cp $ARTIFACT_LOCAL $ARTIFACT_TARGET
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

exit 0