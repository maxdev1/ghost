#!/usr/bin/env bash
set -eo pipefail
source ./ghost.sh

mkdir -p kernel/tests/bin/guest-syscall-memory
"$CROSS_CXX" -std=c++17 -Wall -Wextra -Werror kernel/tests/guest/syscall_memory.cpp \
    -o kernel/tests/bin/guest-syscall-memory/syscall-memory-test.bin

bash tools/testing/build_guest_iso.sh kernel/tests/bin/guest-syscall-memory/syscall-memory-test.bin \
    kernel/tests/bin/guest-syscall-memory/syscall-memory-test.iso

bash tools/testing/build_guest_iso.sh kernel/tests/bin/guest-syscall-memory/syscall-memory-test.bin \
    kernel/tests/bin/guest-syscall-memory/syscall-memory-driver-test.iso --boot-target ahcidriver.bin
