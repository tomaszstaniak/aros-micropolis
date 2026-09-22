#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
demand_test_dir="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-demand.XXXXXX")"
trap 'rm -rf "$demand_test_dir"' EXIT
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I src tests/demand/test.cpp -o "$demand_test_dir/test"
"$demand_test_dir/test"
