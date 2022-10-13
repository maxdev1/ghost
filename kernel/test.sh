#!/bin/bash
ROOT=".."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"


# Flags
with CXX_FLAGS		"-I$SYSROOT/system/include -Isrc -Iinclude -Iinc -fpermissive -w"
with CXX			g++
OBJDIR="bin/obj-test"

# Clean
rm -rf $OBJDIR
mkdir $OBJDIR

# Compile
for file in $(find "src/test" -iname "*.cpp" -o -iname "*.c"); do
	out=`sourceToObject $file`
	list $out
	$CXX -c $file -o "$OBJDIR/$out" $CXX_FLAGS
	failOnError
done

# Link
g++ -o bin/test $OBJDIR/*.o
failOnError

# Execute
./bin/test $1
failOnError
