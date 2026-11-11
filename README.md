# Flashlight

An Android ARM64 LeviLauncher project exploring a customizable first-person flashlight. The intended effect is a colored, soft-edged beam that follows the view with adjustable, natural-looking camera lag while leaving Minecraft's UI unchanged.

## Project status

**This is an early research build, not a working flashlight yet.** The current downloadable `.levipack` is a read-only diagnostic: it reports candidate camera and world-rendering signatures to Android logcat under `FlashlightProbe`. It does not light the world, draw a beam, apply hooks, modify game memory, or implement in-game settings. A standalone quaternion smoothing prototype is covered by host tests but is not connected to Minecraft.

The intended settings include color, brightness, beam size and softness, placement, and camera-follow delay. Rendering these effects independently of the HUD depends on finding and validating the appropriate render pass on the target device. A post-process might brighten existing pixels but is not the same as dynamic world lighting, and may not recover detail in completely black pixels.

## Using the diagnostic

1. Import `Flashlight-Probe.levipack` in LeviLauncher on Android ARM64 and enable it.
2. Start Minecraft, enter a world, and use LogLens or logcat to filter for `FlashlightProbe`.
3. Share the messages showing the five candidate counts and scan completion. A unique match is only a starting point; it does not verify behavior, render backend, ABI, or hook safety.
4. Disable the probe after collecting the log. It may briefly delay library-loading operations while it scans.

The supplied research library identifies itself as `1.26.62.01_RC0`. A version label alone does not establish compatibility with other builds. The diagnostic has not been validated on a device.

## Build

Requirements: a Linux x86-64 host, Python 3, a C++17 compiler for tests, and Android NDK r28c (`28.2.13676358`).

```sh
bash tests/run.sh
ANDROID_NDK_HOME=/path/to/android-ndk-r28c bash tools/build_probe.sh
```

The build produces `artifacts/libFlashlightProbe.so` and `artifacts/Flashlight-Probe.levipack`. GitHub Actions runs the tests and compiles the probe on pushes, pull requests, or manual dispatch. Tags named `probe-v*` trigger a diagnostic prerelease with the `.levipack` as its sole uploaded asset. Build outputs are not committed.

## Credits and license

Copyright © 2026 Zxwmwmwm. Flashlight is MIT licensed. Candidate signatures are derived from [BedrockTools](https://github.com/QYCottage/BedrockTools); its MIT license is included in `third_party/BedrockTools/LICENSE` and the probe package. LeviLauncher and BedrockTools are used as development references. The Minecraft library is not distributed here.
