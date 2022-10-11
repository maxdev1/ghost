#!/bin/bash
ROOT="../.."
if [ -f "$ROOT/variables.sh" ]; then
	. "$ROOT/variables.sh"
fi
. "$ROOT/ghost.sh"

# Creates a pkg-config wrapper from the template
echo "Installing pkg-config wrapper"
OUT_PATH=$TOOLCHAIN_BASE/bin
mkdir -p $OUT_PATH
OUT_FILE=$OUT_PATH/$TARGET-pkg-config.sh
sed -e 's|__SYSROOT__|'$SYSROOT'|' ghost-pkg-config.sh.template > $OUT_FILE 
chmod +x $OUT_FILE
