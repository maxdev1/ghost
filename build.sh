#!/bin/bash
ROOT="."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Colors
red=$'\e[1;31m'
grn=$'\e[1;32m'
yel=$'\e[1;33m'
blu=$'\e[1;34m'
mag=$'\e[1;35m'
cyn=$'\e[1;36m'
wht=$'\e[1;37m'
end=$'\e[0m'

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
		printf "${grn}success${end}\n"
	else
		printf "${red}failed${end}\n"
	fi
}


# Install pkg-config wrapper
pushd tools/pkg-config
$SH build.sh all
popd


# First build necessary ports (if not done yet)
printf "${wht}ports${end} "
if [ -f $SYSROOT/system/lib/libcairo.a ]; then
	printf "${blu}skipped${end}\n"
else
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
printf "${wht}libuser${end} "
if [ -f $SYSROOT/system/lib/libghostuser.a ]; then
	printf "${blu}skipped${end}\n"
else
	pushd libuser
	build_clean
	print_status
	popd
fi

# Build libc & libapi
pushd libapi
printf "${wht}libapi${end} "
build_clean
print_status
popd

pushd libc
printf "${wht}libc${end} "
build_clean
print_status
popd
echo ""

# Make all applications
pushd applications
echo "applications/"
success=0
total=0
for dir in */; do
	printf "  ${wht}${dir%/}${end} "
	pushd $dir
		build_clean
	print_status
		((total=total+1))
	popd
done
echo "($success/$total programs built)"
popd
echo ""


# Finally build kernel
printf "${wht}kernel${end} "
pushd kernel
build_clean
print_status
popd
echo ""

