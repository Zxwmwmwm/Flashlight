#!/usr/bin/env python3
"""Generate an exported, readable notice symbol in the native library."""
import sys
from pathlib import Path

if len(sys.argv) != 2:
    sys.exit("Usage: embed_notices.py output.cpp")

marker = "## BedrockTools license\n\n"
readme = Path("README.md").read_text(encoding="utf-8")
if marker not in readme:
    raise ValueError("BedrockTools license notice missing")
notices = (Path("LICENSE").read_text(encoding="utf-8") + "\n\n" + readme.split(marker, 1)[1]).encode("utf-8")
values = ",".join(str(b) for b in notices + b"\0")
Path(sys.argv[1]).write_text(
    'extern "C" __attribute__((used,visibility("default"))) const char '
    'flashlight_license_notices[] = {' + values + '};\n', encoding="utf-8")
