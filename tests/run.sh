#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/pattern_test.cpp -o "$tmp/pattern_test"
"$tmp/pattern_test"
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/lag_test.cpp -o "$tmp/lag_test"
"$tmp/lag_test"
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Isrc tests/pass_tracker_test.cpp -o "$tmp/pass_tracker_test"
"$tmp/pass_tracker_test"
mkdir -p "$tmp/java"
javac -encoding UTF-8 -d "$tmp/java" src/experiment/UiPolicy.java tests/UiPolicyTest.java
java -cp "$tmp/java" flashlight.UiPolicyTest
