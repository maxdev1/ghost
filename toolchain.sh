#!/bin/bash
if [ -f variables.sh ]; then
	. variables.sh
fi
. ghost.sh

# Define globals
GCC_ARCHIVE=https://ftp.gnu.org/gnu/gcc/gcc-8.2.0/gcc-8.2.0.tar.gz
GCC_PATCH=patches/toolchain/gcc-8.2.0-ghost-1.2.patch
GCC_UNPACKED=gcc-8.2.0

BINUTILS_ARCHIVE=https://ftp.gnu.org/gnu/binutils/binutils-2.31.1.tar.gz
BINUTILS_PATCH=patches/toolchain/binutils-2.31-ghost-1.2.patch
BINUTILS_UNPACKED=binutils-2.31.1

REQUIRED_AUTOCONF="autoconf (GNU Autoconf) 2.64"

TARGET=i686-ghost

STEP_DOWNLOAD=1
STEP_CLEAN=0
STEP_UNPACK=1
STEP_PATCH=1

STEP_BUILD_BINUTILS=1
STEP_BUILD_GCC=1


# Add bin folder to PATH
PATH=$PATH:$TOOLCHAIN_BASE/bin

# Set defaults for variables
with AUTOMAKE	automake
with AUTOCONF	autoconf
with HOST_CXX	g++


# Parse parameters
for var in "$@"; do
	if [ $var == "--skip-archives" ]; then
		STEP_DOWNLOAD=0
		STEP_UNPACK=0
		STEP_PATCH=0
	elif [ $var == "--skip-download" ]; then
		STEP_DOWNLOAD=0
	elif [ $var == "--skip-unpack" ]; then
		STEP_UNPACK=0
	elif [ $var == "--skip-patch" ]; then
		STEP_PATCH=0
	elif [ $var == "--skip-build-binutils" ]; then
		STEP_BUILD_BINUTILS=0
	elif [ $var == "--skip-build-gcc" ]; then
		STEP_BUILD_GCC=0
	elif [ $var == "--clean" ]; then
		STEP_CLEAN=1
	elif [ $var == "--help" ]; then
		echo "Ghost Toolchain setup script"
		echo "Run this script to build a toolchain for your system."
		echo "The following flags are available:"
		echo
		echo "	--skip-download			Skips the downloading of binutils/gcc archives"
		echo "	--skip-unpack			Skips the unpacking of said archives"
		echo "	--skip-patch			Skips the patching of the unpacked sources"
		echo "	--skip-build-binutils		Skips building binutils"
		echo "	--skip-build-gcc		Skips building gcc"
		echo "	--clean				Cleans everything"
		echo
		echo "To specify a different path for your TOOLCHAIN_BASE/SYSROOT,"
		echo "copy 'variables.sh.template' to 'variables.sh' and use export"
		echo "to set the respective variables."
		echo
		exit
	else
		echo "unknown parameter: $var"
		exit
	fi
done

pushd() {
    command pushd "$@" > /dev/null
}

popd() {
    command popd "$@" > /dev/null
}


echo "Checking tools"
requireTool patch
requireTool curl
requireTool $AUTOCONF


# Additional build config
BUILD_GCC_ADDITIONAL_FLAGS=""
case "$(uname -s)" in
	CYGWIN*|MINGW32*|MSYS*)
		echo "Preparing for build on Cygwin"
		BUILD_GCC_ADDITIONAL_FLAGS="--with-gmp=/usr/local --with-mpc=/usr/local --with-mpfr=/usr/local"
		;;
esac



# Check version of autoconf
echo "    $REQUIRED_AUTOCONF"
$AUTOCONF --version | grep -q "$REQUIRED_AUTOCONF"
if [[ $? != 0 ]]; then
	echo "    -> wrong autoconf version:"
	echo "       $($AUTOCONF --version | sed -n 1p)"
	exit
fi


# Print some information
echo "Toolchain will be installed to: $TOOLCHAIN_BASE"





# Clean if necessary
if [ $STEP_CLEAN == 1 ]; then
	rm -rf temp
fi


# Create temp
echo "Creating temporary work directory"
mkdir -p temp


# Download archives if necessary
if [ $STEP_DOWNLOAD == 1 ]; then

	echo "Downloading archive files"
	echo "    gcc"
	curl $GCC_ARCHIVE -o temp/gcc.tar.gz -k
	echo "    binutils"
	curl $BINUTILS_ARCHIVE -o temp/binutils.tar.gz -k
	
else
	echo "Skipping file download"
fi


# Unpack archives
if [ $STEP_UNPACK == 1 ]; then

	echo "Unpacking archives"
	tar -xf temp/gcc.tar.gz -C temp
	failOnError
	tar -xf temp/binutils.tar.gz -C temp
	failOnError
	
else
	echo "Skipping unpacking"
fi


# Apply patches
if [ $STEP_PATCH == 1 ]; then

	echo "Patching GCC"
	patch -d temp/$GCC_UNPACKED -p 1 < $GCC_PATCH				>>temp/patching.log 2>&1
	pushd temp/$GCC_UNPACKED/libstdc++-v3
	echo "Updating autoconf in libstdc++-v3"
	$AUTOCONF
	popd
	
	echo "Patching binutils"
	patch -d temp/$BINUTILS_UNPACKED -p 1 < $BINUTILS_PATCH		>>temp/patching.log 2>&1
	
else
	echo "Skipping patching"
fi


# Build tools
echo "Building 'changes' tool"
pushd tools/changes
CC=$HOST_CXX $SH build.sh all		>>ghost-build.log 2>&1
popd

echo "Building 'ramdisk-writer' tool"
pushd tools/ramdisk-writer
CC=$HOST_CXX $SH build.sh all		>>ghost-build.log 2>&1
popd

echo "Installing 'pkg-config' wrapper"
pushd tools/pkg-config
$SH build.sh						>>ghost-build.log 2>&1
popd


# Install headers
echo "Installing libc and libapi headers"
pushd libc
$SH build.sh install-headers	>>ghost-build.log 2>&1
popd

pushd libapi
$SH build.sh install-headers	>>ghost-build.log 2>&1
popd


# Build binutils
if [ $STEP_BUILD_BINUTILS == 1 ]; then

	echo "Building binutils"
	mkdir -p temp/build-binutils
	pushd temp/build-binutils

	echo "    Configuring"
	../$BINUTILS_UNPACKED/configure --target=$TARGET --prefix=$TOOLCHAIN_BASE --disable-nls --enable-shared --disable-werror --with-sysroot=$SYSROOT >>ghost-build.log 2>&1
	failOnError

	echo "    Building"
	make all -j8						>>ghost-build.log 2>&1
	failOnError

	echo "    Installing"
	make install						>>ghost-build.log 2>&1
	failOnError

	popd

else
	echo "Skipping build of binutils"
fi


# Build gcc
if [ $STEP_BUILD_GCC == 1 ]; then

	echo "Building gcc"
	mkdir -p temp/build-gcc
	pushd temp/build-gcc
	
	echo "    Configuration"
	../$GCC_UNPACKED/configure --target=$TARGET --prefix=$TOOLCHAIN_BASE --disable-nls --enable-languages=c,c++ --enable-shared --with-sysroot=$SYSROOT $BUILD_GCC_ADDITIONAL_FLAGS >>ghost-build.log 2>&1
	failOnError

	echo "    Building core"
	make all-gcc -j8					>>ghost-build.log 2>&1
	failOnError

	echo "    Installing core"
	make install-gcc					>>ghost-build.log 2>&1
	failOnError

	echo "    Building target libgcc"
	make all-target-libgcc -j8			>>ghost-build.log 2>&1
	failOnError

	echo "    Installing target libgcc"
	make install-target-libgcc			>>ghost-build.log 2>&1
	failOnError
	popd

	echo "    Copying artifacts to system/lib"
	cp "$TOOLCHAIN_BASE/i686-ghost/lib/libgcc_s.so.1" "$SYSROOT/system/lib/libgcc_s.so.1"
	failOnError

else
	echo "Skipping build of GCC"
fi


# Build libc
echo "Building libc"
pushd libc
$SH build.sh all						>>ghost-build.log 2>&1
failOnError
popd


# Build libapi
echo "Building libapi"
pushd libapi
$SH build.sh all						>>ghost-build.log 2>&1
failOnError
popd


# Build libstdc++-v3
echo "Building GCC"
pushd temp/build-gcc

echo "    Building libstdc++-v3"
make all-target-libstdc++-v3	>>ghost-build.log 2>&1
failOnError

echo "    Installing libstdc++-v3"
make install-target-libstdc++-v3	>>ghost-build.log 2>&1
failOnError
popd


# Finished
echo "Toolchain successfully built"
