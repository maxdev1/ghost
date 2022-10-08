#!/bin/bash

target_compile() {
	echo "compiling:"

	headers_have_changed=0
	for file in $(find "$SRC" -iname "*.h" -o -iname "*.hpp"); do
		changes -c $file
		if [ $? -eq 1 ]; then
			headers_have_changed=1
		fi
		changes -s $file
	done

	for file in $(find "$SRC" -iname "*.c" -o -iname "*.cpp"); do
		changes -c $file
		changed=$?
		if ([ $headers_have_changed -eq 1 ] || [ $changed -eq 1 ]); then
			out=$(sourceToObject $file)
			list $out
			$CROSS_CXX -c $file -o "$OBJ/$out" $CFLAGS
			failOnError
			changes -s $file
		fi
	done
}
