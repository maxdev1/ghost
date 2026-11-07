#!/usr/bin/env bash
# Run from the repository root inside ghost-toolchain-64.
set -eo pipefail
source ./ghost.sh

mkdir -p kernel/tests/bin/guest-power
"$CROSS_CXX" -std=c++17 -Wall -Wextra -Werror kernel/tests/guest/power/guest.cpp \
    -o kernel/tests/bin/guest-power/power-test.bin

bash tools/testing/build_guest_iso.sh kernel/tests/bin/guest-power/power-test.bin \
    kernel/tests/bin/guest-power/power-test.iso
