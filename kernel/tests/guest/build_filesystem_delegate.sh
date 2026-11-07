#!/usr/bin/env bash
# Run from the repository root inside ghost-toolchain-64.
set -eo pipefail
source ./ghost.sh

mkdir -p kernel/tests/bin/guest-delegate
"$CROSS_CXX" -std=c++17 -Wall -Wextra -Werror kernel/tests/guest/filesystem_delegate.cpp \
     -o kernel/tests/bin/guest-delegate/filesystem-delegate-test.bin

# Boot the test at driver privilege in an isolated ramdisk.
bash tools/testing/build_guest_iso.sh kernel/tests/bin/guest-delegate/filesystem-delegate-test.bin \
    kernel/tests/bin/guest-delegate/filesystem-delegate-test.iso --boot-target ahcidriver.bin
