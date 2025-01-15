#!/bin/bash
ROOT=".."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

TARGET=$@
with TARGET "pack"


#
# Ramdisk build
#
with RAMDISK_WRITER "ramdisk-writer"

#
# ISO file
#
with ISO_SRC        "iso"
with ISO_TGT        "ghost.iso"
with GRUB_MKRESCUE  "grub-mkrescue"

#
# Binaries to copy
#
with KERNEL_BIN     "../kernel/kernel"
with LOADER_BIN     "../kernel/loader"


# Header
target_headline $TARGET


#
# Generate the ramdisk file
#
target_ramdisk() {
	headline "building ramdisk"
	$RAMDISK_WRITER "$SYSROOT" "$ISO_SRC/boot/ramdisk"
	failOnError
}

#
# Build an ISO file using GRUB's mkrescue tool
#
target_make_iso() {
	headline "making iso"
	rm $ISO_TGT
	cp $KERNEL_BIN "$ISO_SRC/boot/kernel"
	cp $LOADER_BIN "$ISO_SRC/boot/loader"

	$GRUB_MKRESCUE --output=$ISO_TGT $ISO_SRC
	failOnError
}

#
# Run in QEMU
#
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

#
# Run in lingemu
#
target_lingemu() {
	lingemu runvirt -m 1024 --diskcontroller type=ahci,name=ahcibus1 --disk $ISO_TGT,disktype=cdrom,controller=ahcibus1
}

#
# Run in QEMU and call debugger
#
target_qemu_debug_gdb() {
	qemu-system-i386 -cdrom $ISO_TGT -s -S -m 1024 -serial tcp::1234,server
}

#
# Only rebuilds the ramdisk and ISO file
#
target_pack() {
	target_ramdisk
	target_make_iso
}


# execute targets
for var in $TARGET; do
	if [[ "$var" == "pack" ]]; then
		target_pack

	elif [[ "$var" == "pack-run" ]]; then
		target_pack
		target_qemu

	elif [[ "$var" == "ramdisk" ]]; then
		target_ramdisk

	elif [[ "$var" == "qemu" ]]; then
		target_qemu

	elif [[ $TARGET == "lingemu" ]]; then
		target_lingemu

	elif [[ "$var" == "qemu-debug-gdb" ]]; then
		target_qemu_debug_gdb

	else
		echo "unknown target: '$var'"
		exit 1
	fi
done

exit 0

