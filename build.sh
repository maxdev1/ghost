#!/bin/bash
source ghost.sh

pushd() {
    command pushd "$@" > /dev/null
}

popd() {
    command popd "$@" > /dev/null
}


# First build necessary ports (if not done yet)
if [ -f $SYSROOT/system/lib/libcairo.a ]; then
	echo "Skipping build of ports"
else
	echo "Building ports"
	pushd patches/ports
	$SH port.sh zlib/1.2.8
	$SH port.sh pixman/0.32.6
	$SH port.sh libpng/1.6.18
	$SH port.sh freetype/2.5.3
	$SH port.sh cairo/1.12.18
	popd
	echo ""
fi


# Prepare C++ library
echo "Building libuser"
pushd libuser
$SH build.sh clean
$SH build.sh all
popd


# Make all applications
pushd applications
for dir in */; do
	echo "Building $dir"
	pushd $dir
		$SH build.sh clean
		$SH build.sh all
	popd
	echo ""
done
popd


# Finally build kernel
echo "Building kernel"
pushd kernel
$SH build.sh clean
$SH build.sh all
popd
echo ""
