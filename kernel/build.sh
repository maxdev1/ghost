#!/bin/sh
. ../ghost.sh

TARGET=$1
with TARGET "all"


INC=inc
BIN=bin
SRC_LOADER=src-loader
SRC_KERNEL=src-kernel
SRC_SHARED=src-shared
INC_LIBGCC=/ghost/lib/gcc/i686-ghost/4.9.1/include

OBJ_SHARED=$BIN/obj-shared
OBJ_LOADER=$BIN/obj-loader
OBJ_KERNEL=$BIN/obj-kernel

ISO_SRC=iso
ISO_TGT=$BIN/image.iso

ARTIFACT_LOADER=$ISO_SRC/boot/loader
ARTIFACT_KERNEL=$ISO_SRC/boot/kernel

LINKSCRIPT_LOADER=extra/link-loader.ld
LINKSCRIPT_KERNEL=extra/link-kernel.ld

RAMDISK=$ISO_SRC/boot/ramdisk

AP_STARTUP_SRC=src-ap/ap_startup.asm
AP_STARTUP_OBJ=$BIN/ap_startup.o
AP_STARTUP_TGT=$SYSROOT/system/lib/apstartup.o

LDFLAGS="-nostdlib -nostartfiles"
CFLAGS="-std=c++11 -D_GHOST_KERNEL_=1 -D_ARCH_X86_=1 -Wall -Wno-unused-but-set-variable -ffreestanding -fno-exceptions -fno-rtti"

QEMU=qemu-system-i386
RAMDISK_WRITER=ramdisk-writer
GRUB_MKRESCUE=grub-mkrescue

# Header
target_headline $TARGET

#
# Cleans binary contents, 
#
target_clean() {
	
	headline "cleaning"
	remove $ARTIFACT_LOADER
	remove $ARTIFACT_KERNEL
	remove $RAMDISK
	remove $ISO_TGT
	cleanDirectory $BIN
	cleanDirectory $OBJ_SHARED
	cleanDirectory $OBJ_LOADER
	cleanDirectory $OBJ_KERNEL
	changes --clear
}

#
# Assembles the application processor startup object.
#
target_compile_ap_startup() {
	headline "building AP startup object"

	$NASM -f bin -o "$AP_STARTUP_OBJ" -s "$AP_STARTUP_SRC"
	failOnError
	mv "$AP_STARTUP_OBJ" "$AP_STARTUP_TGT"
	list $AP_STARTUP_OBJ
}


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


target_link() {
	
	artifact=$1
	script=$2
	objects=$3
	headline "linking $artifact"
	
	$CROSS_LD $LD_FLAGS -o $artifact -T $script $objects
	failOnError
}

target_ramdisk() {
	headline "building ramdisk"
	$RAMDISK_WRITER "$SYSROOT" "$RAMDISK"
	failOnError
}

target_make_iso() {
	headline "making iso"
	$GRUB_MKRESCUE $ISO_SRC --output=$ISO_TGT
	failOnError
}

target_qemu() {
	if [ -e serial.log ]; then
		rm -f serial.log
	fi
	
	echo "Debug interface mode: Make sure the debug application is running!"
	qemu-system-i386 -cdrom $ISO_TGT -s -m 1024 -serial tcp::25000
	
	if [ $? == 1 ]; then
		echo "WARNING: Failed to connect to debug application! Redirecting output to file"
		qemu-system-i386 -cdrom $ISO_TGT -s -m 1024 -serial file:serial.log
	fi
}

target_qemu_debug_gdb() {
	qemu-system-i386 -cdrom $ISO_TGT -s -S -m 1024 -serial tcp::1234,server
}

target_all() {
	target_compile_ap_startup
	target_compile $SRC_SHARED $OBJ_SHARED "-I$INC -I$INC_LIBGCC -I$SRC_SHARED"
	target_compile $SRC_LOADER $OBJ_LOADER "-I$INC -I$INC_LIBGCC -I$SRC_SHARED -I$SRC_LOADER"
	target_compile $SRC_KERNEL $OBJ_KERNEL "-I$INC -I$INC_LIBGCC -I$SRC_SHARED -I$SRC_KERNEL"
	target_link $ARTIFACT_LOADER $LINKSCRIPT_LOADER "$OBJ_LOADER/* $OBJ_SHARED/*"
	target_link $ARTIFACT_KERNEL $LINKSCRIPT_KERNEL "$OBJ_KERNEL/* $OBJ_SHARED/*"
	target_ramdisk
	target_make_iso
}

target_repack() {
	target_ramdisk
	target_make_iso
}


# execute targets
if [[ $TARGET == "all" ]]; then
	target_all

elif [[ $TARGET == "repack" ]]; then
	target_repack
	target_qemu
	
elif [[ $TARGET == "ramdisk" ]]; then
	target_ramdisk
	
elif [[ $TARGET == "qemu" ]]; then
	target_qemu
	
elif [[ $TARGET == "qemu-debug-gdb" ]]; then
	target_qemu_debug_gdb
	
elif [[ $TARGET == "clean" ]]; then
	target_clean
	
else
	echo "unknown target: '$TARGET'"
	exit 1
fi

exit 0
 
