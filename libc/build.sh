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
with INC_API				"../libapi/inc"
with INC_KERNEL				"../kernel/inc"

with ARTIFACT_NAME			"libc.a"
with ARTIFACT_LOCAL			"$ARTIFACT_NAME"
with ARTIFACT_TARGET		"$SYSROOT_SYSTEM_LIB/$ARTIFACT_NAME"
with ARTIFACT_NAME_SHARED	"libc.so"
with ARTIFACT_LOCAL_SHARED	"$ARTIFACT_NAME_SHARED"
with ARTIFACT_TARGET_SHARED	"$SYSROOT_SYSTEM_LIB/$ARTIFACT_NAME_SHARED"

with CFLAGS					"-std=c11 -fpic -I$INC -I$INC_API -I$INC_KERNEL -I$SRC/musl -Wno-narrowing"
with CCFLAGS				"-std=c11 -fpic -I$INC -I$INC_API -I$INC_KERNEL"
with LDFLAGS				"-shared -shared-libgcc"

with CRT_SRC				"crt"
with CRT_OBJ				"crtobj"
CRT_NAMES=("crt0" "crti" "crtn")

CONFORMITY_CHECK="$SRC/_conformity.c"

LINK_STATIC=1
LINK_SHARED=1


echo "target: $TARGET"

# always create output folder
mkdir -p $OBJ
mkdir -p $CRT_OBJ


target_clean() {
	echo "cleaning:"
	rm $ARTIFACT_LOCAL
	rm $ARTIFACT_LOCAL_SHARED
	cleanDirectory $OBJ
	cleanDirectory $CRT_OBJ
	changes --clear
}

target_check_c_conformity() {
	# this checks the C conformity by compiling it with the non-C++ compiler
	echo "checking if headers are C11 conform:"
	out=`sourceToObject $CONFORMITY_CHECK`
	list $out
	$CROSS_CC -c $CONFORMITY_CHECK -o "$OBJ/$out" $CCFLAGS
	failOnError
}

target_compile() {
	echo "compiling:"
	
	# check if headers have changed
	headers_have_changed=0
	for file in $(find "$INC" -iname "*.h"); do
		changes -c $file
		if [ $? -eq 1 ]; then
			headers_have_changed=1
		fi
		changes -s $file
	done
	
	# compile sources
	for file in $(find "$SRC" -iname "*.cpp" -o -iname "*.c"); do
		
		if [ $file == "src/_conformity.c" ]; then
			echo "   (skipping _conformity.c)"
			continue
		fi
		 
		changes -c $file
		changed=$?
		if ( [ $headers_have_changed -eq 1 ] || [ $changed -eq 1 ] ); then
			out=`sourceToObject $file`

			if ( includeInBuild $file ); then
				list $out
				$CROSS_CC -c $file -o "$OBJ/$out" $CFLAGS
				failOnError
				changes -s $file
			fi
		fi
	done

	
	# assemble sources
	for file in $(find "$SRC" -iname "*.s"); do
		
		changes -c $file
		changed=$?
		if ( [ $headers_have_changed -eq 1 ] || [ $changed -eq 1 ] ); then
			out=`sourceToObject $file`

			if ( includeInBuild $file ); then
				list $out
				$CROSS_GAS -s $file -o "$OBJ/$out"
				failOnError
				changes -s $file
			fi
		fi
	done
}

target_assemble_crts() {
	echo "assembling CRTs:"

	for name in ${CRT_NAMES[@]}; do
		list $name
		$CROSS_GAS $CRT_SRC/$ARCH/$name.S -o "$CRT_OBJ/$name.o"
		failOnError
	done
}

target_archive() {
	echo "archiving:"
	if [ $LINK_STATIC = 1 ]; then
		$CROSS_AR -r $ARTIFACT_LOCAL $OBJ/*.o
		failOnError
	fi
	if [ $LINK_SHARED = 1 ]; then
		$CROSS_CC $LDFLAGS -o $ARTIFACT_LOCAL_SHARED $OBJ/*.o
		failOnError
	fi
}

target_install_headers() {
	
	echo "creating header installation directory"
	mkdir -p $SYSROOT_SYSTEM_INCLUDE
	failOnError
	
	echo "installing headers"
	cp -r $INC/* $SYSROOT_SYSTEM_INCLUDE/
	failOnError
	
}
	
target_install() {
	
	target_install_headers
	
	echo "creating lib installation directory"
	mkdir -p $SYSROOT_SYSTEM_LIB
	
	echo "installing artifacts"
	if [ $LINK_STATIC = 1 ]; then
		cp $ARTIFACT_LOCAL $ARTIFACT_TARGET
	fi
	if [ $LINK_SHARED = 1 ]; then
		cp $ARTIFACT_LOCAL_SHARED $ARTIFACT_TARGET_SHARED
	fi
	
	echo "installing crts"
	for name in ${CRT_NAMES[@]}; do
		list $name
		cp $CRT_OBJ/$name.o $SYSROOT_SYSTEM_LIB/$name.o
	done
	
	echo "installing empty libm"
	$CROSS_AR -r $SYSROOT_SYSTEM_LIB/libm.a
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

		target_check_c_conformity
		target_compile
		target_assemble_crts
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
