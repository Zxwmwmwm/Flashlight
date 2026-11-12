#!/usr/bin/env python3
import json
import subprocess
import tempfile
from pathlib import Path
from zipfile import ZipFile

with tempfile.TemporaryDirectory() as temp:
    temp = Path(temp)
    for manifest, soname in [("src/experiment/manifest.json", "libFlashlightTest.so"),
                            ("src/probe/manifest.json", "libFlashlightProbe.so")]:
        library = temp / soname
        library.write_bytes(b"test library")
        output = temp / "test.levipack"
        subprocess.run(["python3", "tools/package.py", manifest, str(library), str(output)], check=True)
        with ZipFile(output) as archive:
            assert set(archive.namelist()) == {"manifest.json", soname}
            assert archive.read(soname) == b"test library"
            data = json.loads(archive.read("manifest.json"))
            assert data["entry"] == soname
            assert "license_notices" not in data
    source = temp / "notices.cpp"
    library = temp / "notices.so"
    subprocess.run(["python3", "tools/embed_notices.py", str(source)], check=True)
    subprocess.run(["c++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror", "-fPIC", "-shared", str(source), "-o", str(library)], check=True)
    binary = library.read_bytes()
    assert Path("LICENSE").read_bytes() in binary
    assert Path("README.md").read_text(encoding="utf-8").split("## BedrockTools license\n\n", 1)[1].encode("utf-8") in binary
    assert b"flashlight_license_notices" in binary
print("minimal levipack and embedded license notice tests passed")
