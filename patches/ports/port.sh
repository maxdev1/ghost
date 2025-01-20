#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

#
# Ghost port installation script
#
# Automatically downloads, unzips, patches and builds packages as they are configured.
# Look at the existing packages for reference.
#
PACKAGE="$1"
BUILD_ROOT="build"
PREFIX="/system"
REQUIRES_INSTALL_IN_SOURCE_DIR=0


# Helper to quit if a process fails
fail() {
	echo "! $1"
	echo
	echo "(Use \"$0 --help\" for more information)"
	exit 1
}


# Checks if help was requested
if [ "$PACKAGE" = "--help" ]; then
	echo ""
	echo "This script has the ability to automatically download, patch, configure and"
	echo "build libraries for the Ghost OS."
	echo ""
	echo "To install a port, specify the name and version to install, for example:"
	echo ""
	echo "   $0 zlib/1.2.8"
	echo ""
	exit
elif [ "$PACKAGE" = "" ]; then
	fail "please supply a package to install"
fi

echo "building package \"$PACKAGE\""

# checks if a tool is available
requireTool() {
	if ! hash $1 2>/dev/null; then
		fail "! missing required tool: '$1'"
	fi
}

# check if package parameter was given
if [ -z "$PACKAGE" ]; then
	fail "please supply a package to install"
fi

# check if curl is installed
requireTool curl

# make sure working directory exists
if [ ! -d "$BUILD_ROOT" ]; then
	echo "creating target build directory: $BUILD_ROOT"
	mkdir "$BUILD_ROOT"
fi

# check if port exists
if [ ! -d "$PACKAGE" ]; then
	fail "port not found!"
fi

# set & clear working directory for build
BUILD_DIR="$BUILD_ROOT/$PACKAGE"
echo "cleaning build directory: $BUILD_DIR"
if [ -d "$BUILD_DIR" ]; then
	rm -rf "$BUILD_DIR"	
fi
mkdir -p "$BUILD_DIR"


# check required port parts
if [ ! -f "$PACKAGE/package.sh" ]; then
	fail "port did not contain package shell"
fi



# include package script
. $PACKAGE/package.sh
if [ -z "$REMOTE_ARCHIVE" ]; then
	fail "port did not specify REMOTE_ARCHIVE"
fi
if [ -z "$ARCHIVE" ]; then
	fail "port did not specify ARCHIVE"
fi
if [ -z "$UNPACKED_DIR" ]; then
	fail "port did not specify UNPACKED_DIR"
fi

# attempt to download source archive
echo "downloading source from '$REMOTE_ARCHIVE'..."
curl "$REMOTE_ARCHIVE" -k -o $BUILD_DIR/$ARCHIVE
if [ $? != 0 ]; then
	fail "unable to download remote archive"
fi

# run port unpack task
$(cd "$BUILD_DIR"; port_unpack  | sed 's/^/    /')

# check if unpacked-dir exists
if [ ! -d "$BUILD_DIR/$UNPACKED_DIR" ]; then
	fail "port did not specify proper unpacked directory"
fi

# patch it
if [ -f "$PACKAGE/patch.diff" ]; then
	echo "applying patch"
	((cd "$BUILD_DIR/$UNPACKED_DIR" && patch -p1) < $PACKAGE/patch.diff) | sed 's/^/    /'
fi

# run port install task
echo "installing port"
BACK=$(pwd)
if [ $REQUIRES_INSTALL_IN_SOURCE_DIR == 1 ]; then
	cd "$BUILD_DIR/$UNPACKED_DIR"
else
	cd "$BUILD_DIR"
	mkdir "build"
	cd "build"
fi
port_install | sed 's/^/    /' >ghost-build.log 2>&1
failOnError
cd $BACK

# clean up
if [ "$DONT_CLEAN_BUILD" = "1" ]; then
	echo "not cleaning up build directory"
else
	rm -rf $BUILD_ROOT/$PACKAGE
fi

# finish successfully
target_successful
exit 0
