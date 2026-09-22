#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
test_build_dir="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-save-build.XXXXXX")"
trap 'rm -rf "$test_build_dir"' EXIT
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -g -fsanitize=address,undefined \
    test.cpp ../../src/save-replace.cpp -o "$test_build_dir/test"
"$test_build_dir/test"
