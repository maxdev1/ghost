#!/usr/bin/env bash
set -eo pipefail
source ./ghost.sh

mkdir -p kernel/tests/bin/guest-process
"$CROSS_CXX" -std=c++17 -Wall -Wextra -Werror kernel/tests/guest/process.cpp \
    -o kernel/tests/bin/guest-process/process-test.bin

bash tools/testing/build_guest_iso.sh kernel/tests/bin/guest-process/process-test.bin \
    kernel/tests/bin/guest-process/process-test.iso
