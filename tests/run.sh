#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/pattern_test.cpp -o "$tmp/pattern_test"
"$tmp/pattern_test"
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/lag_test.cpp -o "$tmp/lag_test"
"$tmp/lag_test"
