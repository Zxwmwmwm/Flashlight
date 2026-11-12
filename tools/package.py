#!/usr/bin/env python3
"""Create a minimal LeviLauncher package while preserving required notices."""
import json
import sys
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


def package(manifest_path: Path, library_path: Path, output_path: Path) -> None:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("entry") != library_path.name:
        raise ValueError("manifest entry does not match the native library")
    marker = "## BedrockTools license\n\n"
    readme = Path("README.md").read_text(encoding="utf-8")
    if marker not in readme:
        raise ValueError("BedrockTools license notice missing")
    manifest["license_notices"] = {
        "Flashlight": Path("LICENSE").read_text(encoding="utf-8"),
        "BedrockTools": readme.split(marker, 1)[1],
    }
    with ZipFile(output_path, "w", ZIP_DEFLATED) as archive:
        archive.writestr("manifest.json", json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
        archive.write(library_path, library_path.name)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit("Usage: package.py manifest.json library.so output.levipack")
    package(*(Path(arg) for arg in sys.argv[1:]))
