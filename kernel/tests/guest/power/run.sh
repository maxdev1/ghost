#!/usr/bin/env bash
set -euo pipefail
TEST_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export HOST_TEST_TIMEOUT=${HOST_TEST_TIMEOUT:-300}
exec bash "$TEST_ROOT/../../../../applications/tests/run.sh" "$TEST_ROOT" "$@"
