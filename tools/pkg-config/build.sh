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
OUT_FILE=$OUT_PATH/i686-ghost-pkg-config.sh
sed -e 's|__SYSROOT__|'$SYSROOT'|' i686-ghost-pkg-config.sh.template > $OUT_FILE 
chmod +x $OUT_FILE
