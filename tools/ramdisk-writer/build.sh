#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

TARGET=$1

with TARGET		"all"
with CC			"g++"
with LD			"g++"
with CFLAGS		"-std=c++11"
with ARTIFACT	"ramdisk-writer"
with SRC		"src"
with INC		"inc"
with BIN		"bin"
with INSTALL_TARGET	"$TOOLCHAIN_BASE/bin"

echo "target: $TARGET"
requireTool $CC
requireTool $LD


target_compile() {
	echo "compiling:"
	for src in $(find "$SRC" -iname "*.cpp"); do 
		obj=`sourceToObject $src`
		list $obj
		$CC -c $src -o "$BIN/$obj" -I$INC $CFLAGS
		failOnError
	done
}

target_link() {
	echo "linking:"
	$LD -o $ARTIFACT $BIN/*.o
	failOnError
	list $ARTIFACT
}

target_clean() {
	echo "cleaning:"
	cleanDirectory $BIN
}

target_install() {
	echo "installing to '$INSTALL_TARGET':"
	cp $ARTIFACT $INSTALL_TARGET
	failOnError
	list $ARTIFACT
}



if [[ $TARGET == "all" ]]; then
	target_clean
	target_compile
	target_link
	target_install
	
elif [[ $TARGET == "clean" ]]; then
	target_clean
	
else
	echo "unknown target: '$TARGET'"
	exit 1
fi

exit 0