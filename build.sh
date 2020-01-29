#!/bin/bash
ROOT="."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


# Define some helpers
pushd() {
    command pushd "$@" > /dev/null
}

popd() {
    command popd "$@" > /dev/null
}

build_clean() {
	$SH build.sh clean > /dev/null 2>&1 && $SH build.sh all > /dev/null 2>&1
}

print_status() {
	if [ $? -eq 0 ]; then
		printf "success\n"
	else
		printf "\e[1;31mfailed\e[0m\n"
	fi
}

print_skipped() {
	printf "skipped\n"
}

print_name() {
	printf "\e[0;7m$1\e[0m "
}

# Print header
echo ""
printf "\e[4mGhost complete build\e[0m\n"
echo "This script will attempt building all necessary user-space software."
echo "If a part of this build fails, run the respective build script within"
echo "the programs folder to see a more detailed log."
echo ""

# First build necessary ports (if not done yet)
print_name ports
if [ -f $SYSROOT/system/lib/libcairo.a ]; then
	print_skipped
else
	pushd patches/ports
	$SH port.sh zlib/1.2.8
	$SH port.sh pixman/0.32.6
	$SH port.sh libpng/1.6.18
	$SH port.sh freetype/2.5.3
	$SH port.sh cairo/1.12.18
	popd
fi

# Prepare libghostuser
print_name libuser
if [ -f $SYSROOT/system/lib/libghostuser.a ]; then
	print_skipped
else
	pushd libuser
	build_clean
	print_status
	popd
fi

# Prepare libapi & libc
pushd libapi
print_name libapi
build_clean
print_status
popd
pushd libc
print_name libc
build_clean
print_status
popd

# Make all applications
pushd applications
print_name applications
success=0
total=0
for dir in */; do
	pushd $dir
	build_clean
	if [ $? -eq 0 ]; then
		printf "${dir%/} "
		((success=success+1))
	else
		printf "\e[1;31m${dir%/}\e[0m "
	fi
	((total=total+1))
	popd
done
echo "($success/$total programs built)"
popd

# Finally build kernel
print_name kernel
pushd kernel
build_clean
print_status
popd
echo ""
