#!/bin/bash
# Ghost common build functionality


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
	echo " - '$1'"	
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
# Utils
#
pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

# Global variables
with CROSS_HOST			"i686-ghost"
with CROSS_CC			$CROSS_HOST"-gcc"
with CROSS_CXX			$CROSS_HOST"-g++"
with CROSS_LD			$CROSS_HOST"-ld"
with CROSS_GAS			$CROSS_HOST"-as"
with CROSS_AR			$CROSS_HOST"-ar"

with TOOLCHAIN_BASE		"/ghost"
with SYSROOT			$TOOLCHAIN_BASE"/sysroot"

with NASM				nasm
with SH					bash


SYSROOT_APPLICATIONS=$SYSROOT/applications
SYSROOT_SYSTEM=$SYSROOT/system
SYSROOT_SYSTEM_INCLUDE=$SYSROOT_SYSTEM/include
SYSROOT_SYSTEM_LIB=$SYSROOT_SYSTEM/lib

export PATH=$PATH:$TOOLCHAIN_BASE/bin
