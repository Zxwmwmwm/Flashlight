#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to Android NDK r28c}"
compiler="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++"
w=$(mktemp -d); trap 'rm -rf "$w"' EXIT
mkdir -p artifacts
python3 tools/embed_notices.py "$w/notices.cpp"
"$compiler" -std=c++17 -O2 -Wall -Wextra -Werror -fPIC -shared -Wl,-z,max-page-size=16384 \
    src/probe/probe.cpp "$w/notices.cpp" -llog -ldl -static-libstdc++ -o artifacts/libFlashlightProbe.so
python3 tools/package.py src/probe/manifest.json artifacts/libFlashlightProbe.so artifacts/Flashlight-Probe.levipack
