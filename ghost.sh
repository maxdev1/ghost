#!/bin/bash

#
# Ghost common build functionality
#
(return 0 2>/dev/null) && sourced=1 || sourced=0
if [[ $sourced = 0 ]]; then
	echo "This file contains definitions used throughout the build. Use 'toolchain.sh' and 'build.sh' instead."
fi

# Include user variables if available
if [ -f variables.sh ]; then
	. variables.sh
fi

#
# Overrides a variable if it is not set
#
#   with CC "g++"
#
with() {
	if [ -z "${!1}" ]; then eval $1='$2'; fi
}

#
# Requires the specified parameter, quits with an error otherwise
#
#   requireVar "ARTIFACT_NAME"
#
requireVar() {
	name=$1
	if [ -z "${!name}" ]
	then
		>&2 echo "error: missing required parameter '$1'"
		exit 1
	fi
}

#
# Requires the specified tool, quits with an error otherwise
#
#   requireTool "g++"
#
requireTool() {
	if ! type "$1" >/dev/null 2>&1
	then
		>&2 echo "error: missing required tool '$1'"
		exit 1
	fi
}

#
# Prints a list entry
#
#   list "example.txt"
#
list() {
	echo " - $1"	
}

#
# Cleans the given directory
#
#   cleanDirectory "bin"
#
cleanDirectory() {
	if [ -d "$1" ]
	then
		rm -rf $1
	fi
	mkdir "$1"
	list $1
}

#
# Removes the given file
#
#	remove "file.txt"
#
remove() {
	if [ -e "$1" ]
	then
		rm $1 2&> /dev/null
	fi
	list $1
}

#
# Fails if the previous command was erroneous
#
#   $CC -c ...
#   failOnError
#
failOnError() {
	if [[ $? != 0 ]]; then
		printf "\e[31;1mtarget failed\e[0m\n\n"	
		exit 1
	fi	
}

#
# Converts a source path to an object
#
#   sourceToObject "src/myclass.cpp"
#
sourceToObject() {
	echo $1.o | sed "s/\//_/g"
}


#
# Prints a header
#
headline() {
	printf "\e[0;1m$1\e[0m:\n"
}

target_headline() {
	printf "\e[0;7mTARGET:\e[0m $1\n"
}

target_successful() {
	printf "\e[0;1mtarget successful\e[0m\n\n"	
}

#
# Check if an entry exists in a list
#
#	findInList "a b c" " " "a"
#
findInList() {
    [[ "$1" =~ ($2|^)$3($2|$) ]]
}

#
# Utils
#
pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}


# Global variables
with TARGET				"x86_64-ghost"
with CROSS_CC			$TARGET"-gcc"
with CROSS_CXX		$TARGET"-g++"
with CROSS_LD			$TARGET"-ld"
with CROSS_GAS		$TARGET"-as"
with CROSS_AR			$TARGET"-ar"

# Limine
with LIMINE_VERSION "9.2.0"
with LIMINE_SOURCE "https://ghostkernel.org/repository/limine/limine-$LIMINE_VERSION.tar.gz"

# Target architecture
__TARGET_ARCH_PART=${TARGET%-*}
if [[ $__TARGET_ARCH_PART == "i686" ]]; then
	ARCH="i386"
elif [[ $__TARGET_ARCH_PART == "x86_64" ]]; then
	ARCH="x86_64"
else
	echo "Unsupported architecture: $__TARGET_ARCH_PART"
	exit 1
fi

SUPPORTED_ARCHS="i386 x86_64"

#
# Checks whether the file should be included in the build.
#
includeInBuild() {
	fileArch=$(basename $(dirname $file))
	if findInList "$SUPPORTED_ARCHS" " " "$fileArch"; then
		if [[ $ARCH == $fileArch ]]; then
			return 0
		else
			return 1
		fi
	fi
	return 0
}


# Toolchain specifics
with TOOLCHAIN_BASE		"/ghost"
with SYSROOT			    $TOOLCHAIN_BASE"/source/sysroot"

with NASM				nasm
with SH					bash


SYSROOT_APPLICATIONS=$SYSROOT/applications
SYSROOT_SYSTEM=$SYSROOT/system
SYSROOT_SYSTEM_INCLUDE=$SYSROOT_SYSTEM/include
SYSROOT_SYSTEM_LIB=$SYSROOT_SYSTEM/lib

export PATH=$PATH:$TOOLCHAIN_BASE/bin
