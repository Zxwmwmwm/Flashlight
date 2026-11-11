#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
w=$(mktemp -d);trap 'rm -rf "$w"' EXIT
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Itests/host_include tests/render_test.cpp -lEGL -lGLESv2 -ldl -pthread -o "$w/render_test"
"$w/render_test"
