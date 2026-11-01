#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

ARTIFACT_NAME="acpidriver.bin"
CFLAGS_ADD="-O2 -Wall -Wextra -Werror -Iuacpi/include -DUACPI_USE_BUILTIN_STRING"

actions="$*"
if [ -z "$actions" ]; then actions=all; fi
for action in $actions; do
	if [ "$action" = all ]; then
		mkdir -p obj
		for source in uacpi/source/*.c; do
			"$CROSS_CC" -std=c11 -O2 -Wall -Wextra -Iuacpi/include \
				-DUACPI_USE_BUILTIN_STRING -c "$source" -o "obj/uacpi_$(basename "$source").o"
			failOnError
		done
	fi
	(. "../applications.sh" "$action")
	failOnError
done
