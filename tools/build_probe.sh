#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to Android NDK r28c}"
compiler="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++"
mkdir -p artifacts
"$compiler" -std=c++17 -O2 -Wall -Wextra -Werror -fPIC -shared -Wl,-z,max-page-size=16384 \
    src/probe/probe.cpp -llog -ldl -static-libstdc++ -o artifacts/libFlashlightProbe.so
python3 - <<'PY'
from zipfile import ZipFile, ZIP_DEFLATED
with ZipFile('artifacts/Flashlight-Probe.levipack', 'w', ZIP_DEFLATED) as z:
    for source, target in [('src/probe/manifest.json', 'manifest.json'),
                           ('artifacts/libFlashlightProbe.so', 'libFlashlightProbe.so'),
                           ('LICENSE', 'LICENSE')]:
        z.write(source, target)
    notice=open('README.md',encoding='utf-8').read().split('## BedrockTools license\n\n',1)[1]
    z.writestr('LICENSE-BedrockTools',notice)
PY
