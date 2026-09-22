#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
test_build_dir="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-classic-tools.XXXXXX")"
trap 'rm -rf "$test_build_dir"' EXIT
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-sanitize-recover=all -I src tests/classic-tools/test.cpp -o "$test_build_dir/test"
"$test_build_dir/test"

python3 tests/classic-tools/popup-integration.py
