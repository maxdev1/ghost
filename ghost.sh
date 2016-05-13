#!/bin/bash
# Ghost common build functionality



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
		echo "Build failed"
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
	printf "TARGET: \e[0;7m$1\e[0m\n\n"	
}


# Global variables
with CROSS_PREFIX		"i686-ghost-"
with CROSS_CC			$CROSS_PREFIX"gcc"
with CROSS_CXX			$CROSS_PREFIX"g++"
with CROSS_LD			$CROSS_PREFIX"ld"
with CROSS_GAS			$CROSS_PREFIX"as"
with CROSS_AR			$CROSS_PREFIX"ar"
with NASM				"nasm"
with TOOLCHAIN_BASE		"/ghost"

with SYSROOT			$TOOLCHAIN_BASE"/sysroot"
SYSROOT_APPLICATIONS=$SYSROOT/applications
SYSROOT_SYSTEM=$SYSROOT/system
SYSROOT_SYSTEM_INCLUDE=$SYSROOT_SYSTEM/include
SYSROOT_SYSTEM_LIB=$SYSROOT_SYSTEM/lib
