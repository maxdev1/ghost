#!/usr/bin/env bash
set -eo pipefail
source ./ghost.sh

mkdir -p kernel/tests/bin/guest-tls
"$CROSS_CXX" -std=c++17 -Wall -Wextra -Werror -fPIC -shared kernel/tests/guest/tls_library.cpp \
    -Wl,-soname,libtls-test.so -o kernel/tests/bin/guest-tls/libtls-test.so
"$CROSS_CXX" -std=c++17 -Wall -Wextra -Werror kernel/tests/guest/tls.cpp \
    -Lkernel/tests/bin/guest-tls -ltls-test -o kernel/tests/bin/guest-tls/tls-test.bin

bash tools/testing/build_guest_iso.sh kernel/tests/bin/guest-tls/tls-test.bin \
    kernel/tests/bin/guest-tls/tls-test.iso kernel/tests/bin/guest-tls/libtls-test.so
