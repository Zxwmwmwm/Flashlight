#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${ANDROID_NDK_HOME:?Set Android NDK r28c path}"
: "${ANDROID_JAR:?Set Android platform android.jar path}"
: "${R8_JAR:?Set D8 R8 jar path}"
w=$(mktemp -d); trap 'rm -rf "$w"' EXIT
mkdir -p "$w/classes" "$w/dex" artifacts
javac -encoding UTF-8 -source 8 -target 8 -classpath "$ANDROID_JAR" -d "$w/classes" src/experiment/FlashlightTest.java src/experiment/UiPolicy.java
(cd "$w/classes" && python3 -m zipfile -c "$w/ui.jar" flashlight)
java -cp "$R8_JAR" com.android.tools.r8.D8 --min-api 26 --lib "$ANDROID_JAR" --output "$w/dex" "$w/ui.jar"
python3 - "$w/dex/classes.dex" "$w/dex.cpp" <<'PY'
import sys
b=open(sys.argv[1],'rb').read()
with open(sys.argv[2],'w') as f:f.write('extern const unsigned char dex_data[] = {'+','.join(map(str,b))+'};\nextern const unsigned int dex_len = '+str(len(b))+';\n')
PY
"$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++" -std=c++17 -O2 -Wall -Wextra -Werror -fPIC -shared -Wl,-z,max-page-size=16384 src/experiment/loader.cpp src/experiment/engine.cpp "$w/dex.cpp" -llog -ldl -lEGL -lGLESv3 -static-libstdc++ -o artifacts/libFlashlightTest.so
python3 - <<'PY'
from zipfile import ZipFile,ZIP_DEFLATED
with ZipFile('artifacts/Flashlight.levipack','w',ZIP_DEFLATED) as z:
 for s,t in [('src/experiment/manifest.json','manifest.json'),('artifacts/libFlashlightTest.so','libFlashlightTest.so'),('LICENSE','LICENSE')]:z.write(s,t)
 z.writestr('LICENSE-BedrockTools',open('README.md',encoding='utf-8').read().split('## BedrockTools license\n\n',1)[1])
PY
