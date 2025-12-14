#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="${ROOT_DIR}/build-deps"
PREFIX="${AUTOCONF_PREFIX:-${WORK_DIR}/autoconf-2.69}"
ARCHIVE_NAME="autoconf-2.69.tar.gz"
ARCHIVE_PATH="${WORK_DIR}/${ARCHIVE_NAME}"
SOURCE_URL="https://ftp.gnu.org/gnu/autoconf/${ARCHIVE_NAME}"
SRC_DIR="${WORK_DIR}/autoconf-2.69-src"

mkdir -p "${WORK_DIR}"

echo "[autoconf-2.69] Downloading to ${ARCHIVE_PATH}"
curl -L "${SOURCE_URL}" -o "${ARCHIVE_PATH}"

rm -rf "${SRC_DIR}"
mkdir -p "${SRC_DIR}"
tar -xzf "${ARCHIVE_PATH}" -C "${SRC_DIR}" --strip-components=1

pushd "${SRC_DIR}" >/dev/null
echo "[autoconf-2.69] Configuring with prefix ${PREFIX}"
./configure --prefix="${PREFIX}"
echo "[autoconf-2.69] Building"
make -j"$(nproc)"
echo "[autoconf-2.69] Installing"
make install
popd >/dev/null

cat <<EOF

Autoconf 2.69 installed under: ${PREFIX}

Before running toolchain.sh or bootstrap-toolchain.sh, prepend this to PATH:
  export PATH=${PREFIX}/bin:\$PATH

EOF
