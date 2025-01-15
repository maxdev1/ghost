#!/bin/bash
ROOT=".."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


TARGET=$@
with TARGET "all"

#
# Source directories
#
INC=inc
BIN=bin
SRC=src
SRC_LOADER=$SRC/loader
SRC_KERNEL=$SRC/kernel
SRC_SHARED=$SRC/shared

#
# Compiler flags
#
LDFLAGS="-nostdlib -nostartfiles"
CFLAGS="-std=c++11 -D_GHOST_KERNEL_=1 -Wall -Wno-unused-but-set-variable -ffreestanding -fno-exceptions -fno-rtti"

#
# Object output folders
#
OBJ_SHARED=$BIN/obj-shared
OBJ_LOADER=$BIN/obj-loader
OBJ_KERNEL=$BIN/obj-kernel


#
# Generated artifacts & linker scripts
#
ARTIFACT_LOADER=loader
ARTIFACT_KERNEL=kernel
LINKSCRIPT_LOADER=extra/link-loader.ld
LINKSCRIPT_KERNEL=extra/link-kernel.ld


#
# Application processor startup object
#
AP_STARTUP_SRC=src/ap/ap_startup.asm
AP_STARTUP_OBJ=$BIN/ap_startup.o
AP_STARTUP_TGT=$SYSROOT/system/lib/apstartup.o


# Header
target_headline $TARGET

#
# Cleans output folders and files
#
target_clean() {
	
	headline "cleaning"
	remove $ARTIFACT_LOADER
	remove $ARTIFACT_KERNEL
	cleanDirectory $BIN
	cleanDirectory $OBJ_SHARED
	cleanDirectory $OBJ_LOADER
	cleanDirectory $OBJ_KERNEL
	changes --clear
}

#
# Assemble the application processor startup object
#
target_compile_ap_startup() {
	headline "building AP startup object"

	$NASM -f bin -o "$AP_STARTUP_OBJ" -s "$AP_STARTUP_SRC"
	failOnError
	mv "$AP_STARTUP_OBJ" "$AP_STARTUP_TGT"
	list $AP_STARTUP_OBJ
}

#
# Compile all sources that have changed in the given directory
#
target_compile() {
	
	srcdir=$1
	objdir=$2
	includes=$3
	headline "compiling $srcdir"
	
	# check if headers have changed
	headers_have_changed=0
	for file in $(find "$INC" -iname "*.hpp" -o -iname "*.h"); do
		changes -c $file
		if [ $? -eq 1 ]; then
			headers_have_changed=1
		fi
		changes -s $file
	done
	
	# compile sources
	for file in $(find "$srcdir" -iname "*.cpp" -o -iname "*.c"); do
		
		changes -c $file
		changed=$?
		if ( [ $headers_have_changed -eq 1 ] || [ $changed -eq 1 ] ); then
			out=`sourceToObject $file`
			list $out
			$CROSS_CXX -c $file -o "$objdir/$out" $includes $CFLAGS
			failOnError
			changes -s $file
		fi
	done

	# assemble sources
	for file in $(find "$srcdir" -iname "*.asm"); do
		
		changes -c $file
		changed=$?
		if ( [ $headers_have_changed -eq 1 ] || [ $changed -eq 1 ] ); then
			out=`sourceToObject $file`
			list $out
			$NASM -f elf -s $file -o "$objdir/$out"
			failOnError
			changes -s $file
		fi
	done
}

#
# Link an artifact
#
target_link() {
	
	artifact=$1
	script=$2
	objects=$3
	headline "linking $artifact"
	
	$CROSS_LD $LD_FLAGS -o $artifact -T $script $objects
	failOnError
}

#
# Performs everything necessary to build the OS image
#
target_all() {
	target_compile_ap_startup
	target_compile $SRC_SHARED $OBJ_SHARED "-I$INC -I$SRC"
	target_compile $SRC_LOADER $OBJ_LOADER "-I$INC -I$SRC"
	target_compile $SRC_KERNEL $OBJ_KERNEL "-I$INC -I$SRC"
	target_link $ARTIFACT_LOADER $LINKSCRIPT_LOADER "$OBJ_LOADER/* $OBJ_SHARED/*"
	target_link $ARTIFACT_KERNEL $LINKSCRIPT_KERNEL "$OBJ_KERNEL/* $OBJ_SHARED/*"
}


# execute targets
for var in $TARGET; do
	if [[ "$var" == "all" ]]; then
		target_all

	elif [[ "$var" == "clean" ]]; then
		target_clean
		
	else
		echo "unknown target: '$var'"
		exit 1
	fi
done

exit 0
 
