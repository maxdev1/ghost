#!/bin/bash
set -euo pipefail

if [ -z "${SYSROOT:-}" ]; then
  echo "fix-la.sh: SYSROOT not set" >&2
  exit 1
fi

LIBDIR="${SYSROOT}/system/lib"
[ -d "${LIBDIR}" ] || exit 0

escaped_sysroot=$(printf '%s\n' "${SYSROOT}" | sed 's/[.[\*^$(){}?+|/]/\\&/g')
find "${LIBDIR}" -name '*.la' -print0 | while IFS= read -r -d '' la; do
  perl -0pi -e "s#(?<!${escaped_sysroot})/system#${SYSROOT}/system#g" "${la}"
done
