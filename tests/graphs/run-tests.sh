#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
graph_test_dir="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-graphs.XXXXXX")"
trap 'rm -rf "$graph_test_dir"' EXIT
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -I src tests/graphs/test.cpp -o "$graph_test_dir/test"
"$graph_test_dir/test"
python3 tests/graphs/integration.py
