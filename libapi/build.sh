#!/bin/bash
ROOT=".."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


TARGET=$@

with TARGET					"all"
with SRC					"src"
with OBJ					"obj"
with INC					"inc"

with ARTIFACT_NAME			"libghostapi.a"
with ARTIFACT_LOCAL			"$ARTIFACT_NAME"
with ARTIFACT_TARGET		"$SYSROOT_SYSTEM_LIB/$ARTIFACT_NAME"
with ARTIFACT_NAME_SHARED	"libghostapi.so"
with ARTIFACT_LOCAL_SHARED	"$ARTIFACT_NAME_SHARED"
with ARTIFACT_TARGET_SHARED	"$SYSROOT_SYSTEM_LIB/$ARTIFACT_NAME_SHARED"

with CFLAGS					"-std=c++11 -fpic -I$INC"
with LDFLAGS				"-shared -shared-libgcc"

LINK_STATIC=1
LINK_SHARED=1


echo "target: $TARGET"

if [[ "$TARGET" != "install-headers" ]]; then
	requireTool changes
fi

# always create output folder
mkdir -p $OBJ


target_clean() {
	echo "cleaning:"
	rm $ARTIFACT_LOCAL
	rm $ARTIFACT_LOCAL_SHARED
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
	if [ $LINK_STATIC = 1 ]; then
		$CROSS_AR -r $ARTIFACT_LOCAL $OBJ/*.o
	fi
	if [ $LINK_SHARED = 1 ]; then
		$CROSS_CC $LDFLAGS -o $ARTIFACT_LOCAL_SHARED $OBJ/*.o
	fi
}
	
target_clean_target() {
	if [ $LINK_STATIC = 1 ]; then
		echo "removing $ARTIFACT_TARGET"
		rm $ARTIFACT_TARGET 2&> /dev/null
	fi
	if [ $LINK_SHARED = 1 ]; then
		echo "removing $ARTIFACT_TARGET_SHARED"
		rm $ARTIFACT_TARGET_SHARED 2&> /dev/null
	fi
}

target_install_headers() {
	echo "installing api headers"
	cp -r $INC/* $SYSROOT_SYSTEM_INCLUDE/
	failOnError
}

target_install() {
	target_clean_target
	target_install_headers
	
	echo "installing artifacts"
	if [ $LINK_STATIC = 1 ]; then
		cp $ARTIFACT_LOCAL $ARTIFACT_TARGET
	fi
	if [ $LINK_SHARED = 1 ]; then
		cp $ARTIFACT_LOCAL_SHARED $ARTIFACT_TARGET_SHARED
	fi
}


# execute targets
for var in $TARGET; do
	if [[ $var == "install-headers" ]]; then
		target_install_headers

	elif [[ $var == "all" || $var == "static" || $var == "shared" ]]; then
		if [[ $var == "static" ]]; then
			LINK_SHARED=0
		elif [[ $var == "shared" ]]; then
			LINK_STATIC=0
		fi

		target_compile
		target_archive
		target_install
		
	elif [[ $var == "clean" ]]; then
		target_clean
		
	else
		echo "unknown target: '$var'"
		exit 1
	fi
done

target_successful
exit 0
