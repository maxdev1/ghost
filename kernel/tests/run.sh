#!/usr/bin/env bash
set -euo pipefail

TEST_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPOSITORY=$(cd "$TEST_ROOT/../.." && pwd)
if [[ ! -f "$REPOSITORY/third_party/googletest/googletest/include/gtest/gtest.h" ]]; then
    echo 'Missing GoogleTest. Run: git submodule update --init third_party/googletest' >&2
    exit 1
fi
SANITIZE=${SANITIZE:-1}
if [[ "$SANITIZE" != 0 && "$SANITIZE" != 1 ]]; then
    echo 'SANITIZE must be 0 or 1.' >&2
    exit 1
fi
BUILD_DIR=${KERNEL_TEST_BUILD_DIR:-"$TEST_ROOT/bin/$(uname -s)-$(uname -m)-sanitize$SANITIZE"}
if [[ "$BUILD_DIR" != /* ]]; then
    echo 'KERNEL_TEST_BUILD_DIR must be an absolute path.' >&2
    exit 1
fi
make -C "$TEST_ROOT" -j"${JOBS:-4}" BUILD_DIR="$BUILD_DIR" SANITIZE="$SANITIZE" all

tests=()
for source in "$TEST_ROOT"/*/*_test.cpp; do
    [[ -f "$source" ]] || continue
    name=${source#"$TEST_ROOT/"}
    name=${name%.cpp}
    if [[ "$name" == ${KERNEL_TEST_FILTER:-*} ]]; then tests+=("$BUILD_DIR/$name"); fi
done
if [[ ${#tests[@]} -eq 0 ]]; then
    echo "No kernel tests match '${KERNEL_TEST_FILTER:-*}'." >&2
    exit 1
fi
exec "$BUILD_DIR/run_tests" "${tests[@]}" -- "$@"
