#!/bin/bash
ROOT=".."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


# Add ignore cast-error on Mac
unameOut="$(uname -s)"
case "${unameOut}" in
    Darwin*)    CXX_FLAGS=" -fms-extensions";;
esac

# Flags
with CXX_FLAGS		""
with CXX			g++
OBJDIR="bin/obj-test"

# Clean
rm -rf $OBJDIR
mkdir $OBJDIR

# Compile
for file in $(find "src/test" -iname "*.cpp" -o -iname "*.c"); do
	out=`sourceToObject $file`
	list $out
	$CXX -c $file -o "$OBJDIR/$out" -Isrc -Iinclude -Iinc -fpermissive -w $CXX_FLAGS
	failOnError
done

# Link
gcc -o bin/test $OBJDIR/*.o
failOnError

# Execute
./bin/test
failOnError
