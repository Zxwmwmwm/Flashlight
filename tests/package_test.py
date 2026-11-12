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
            assert "Zxwmwmwm" in data["license_notices"]["Flashlight"]
            assert "RadiantByte" in data["license_notices"]["BedrockTools"]
            assert "Permission is hereby granted" in data["license_notices"]["BedrockTools"]
print("minimal levipack and license notice tests passed")
