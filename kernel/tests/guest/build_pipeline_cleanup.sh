#!/usr/bin/env bash
set -eo pipefail
source ./ghost.sh

mkdir -p kernel/tests/bin/guest-pipeline
"$CROSS_CXX" -std=c++17 -Wall -Wextra -Werror kernel/tests/guest/pipeline_cleanup.cpp \
    -o kernel/tests/bin/guest-pipeline/pipeline-cleanup-test.bin

bash tools/testing/build_guest_iso.sh kernel/tests/bin/guest-pipeline/pipeline-cleanup-test.bin \
    kernel/tests/bin/guest-pipeline/pipeline-cleanup-test.iso --boot-target ahcidriver.bin
