#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
overview_test_dir="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-overview.XXXXXX")"
trap 'rm -rf "$overview_test_dir"' EXIT
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I src tests/overview/test.cpp -o "$overview_test_dir/test"
"$overview_test_dir/test"
