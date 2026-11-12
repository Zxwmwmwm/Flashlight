# Flashlight

A customizable camera-following flashlight for LeviLauncher on Android ARM64, by **Zxwmwmwm**.

## Features

- Soft adjustable spotlight with color, brightness, size, softness, position and camera-follow delay.
- Optional **Visible cone** with independent horizontal and vertical origin sliders. Its color and strength follow the main beam, and the shaft extends across the sky.
- Optional **Dark world** mode with a separate brightness control.
- Optional depth-based **Sky masking**, enabled by default.
- Movable, resizable control panel and optional pause-only controls. Settings are saved on the device.

## Install and use

1. Download `Flashlight.levipack` from the [latest release](https://github.com/Zxwmwmwm/Flashlight/releases/latest), import it into LeviLauncher and enable it. Disable earlier Flashlight test or probe builds and other rendering-hook mods before trying this release.
2. Enter a world, tap the floating **FL** button, then **Turn beam on**. The beam starts off each time the mod starts and runs only when the launcher reports the gameplay HUD.
3. Turn **Visible cone** on to show the light shaft. Use **Cone horizontal origin** (left to right) and **Cone vertical origin** (top to bottom) to move where it begins. The separately labeled **Beam** position sliders move the spotlight aim. The cone defaults to the lower-right and is off until enabled.
4. Use the other sliders to customize the beam. Turn on **Dark world** to use the saved dark-world brightness. Use **Sky masking** to suppress the spotlight on far-background pixels while keeping the visible cone in the sky; turn it off if it hides the spotlight incorrectly. When depth copying is unavailable, the panel reports it and rendering continues without the mask.
5. Drag the panel by its title, resize it with **↘ RESIZE**, and optionally enable **Pause-only controls**. Pause the game to reveal controls again; if pause detection is unavailable, controls stay accessible.

## Compatibility and limitations

Flashlight uses a screen-space GLES effect, not physical world lighting. Its visible cone is a stylized overlay, not a held object or true volumetric beam. It cannot cast physical shadows, illuminate off-screen blocks, recover detail from completely black pixels, or reliably account for every occluding surface. Sky masking depends on a usable depth buffer and can misclassify sky or distant geometry. The visible cone uses the depth at the spotlight aim to decide whether to extend into the sky; it is a stylized approximation and does not follow physical occlusion. Without depth access, it uses the extended profile everywhere. In third person the effect remains camera-relative rather than attached to the player. Render order can also affect the hand or HUD, and other graphics backends may not support the effect.

The mod uses native camera, GLES and EGL hooks, gated to the exact Android game library with GNU build ID `c60fa32908019eecae39ebfe7bac5755dc35cde2` and matching signatures, associated with the supplied 1.26.62 build. Similar version labels alone do not establish compatibility. The checks do not guarantee hook safety across devices or graphics configurations. If you encounter a crash or visual issue, disable the mod and restart Minecraft; logs are tagged `FlashlightTest`.

## Build and tests

Requirements: Linux x86-64, Python 3, C++17, JDK, Android NDK r28c, Android platform `android.jar`, and a D8/R8 jar.

```sh
bash tests/run.sh
# Optional software EGL/GLES test (development headers and libraries required):
bash tests/run_render.sh
export ANDROID_NDK_HOME=/path/to/android-ndk-r28c
export ANDROID_JAR=/path/to/android.jar
export R8_JAR=/path/to/d8.jar
bash tools/build_test.sh
# Optional read-only diagnostic probe:
bash tools/build_probe.sh
```

Packages contain only the native `.so` and `manifest.json`; the required MIT notices are included in the manifest. Packages are generated under `artifacts/`, not committed to source. See [release notes](RELEASE_NOTES.md) for the latest changes. GitHub Actions runs tests and builds ARM64 packages. A `v*` tag publishes a regular release with `Flashlight.levipack` as the sole uploaded asset; `test-v*` and `probe-v*` tags publish their respective prereleases. The separate diagnostic probe scans signatures and logs match counts without rendering or installing hooks.

## Credits and license

Copyright © 2026 Zxwmwmwm. Flashlight is MIT licensed. Candidate signatures, camera offset, and render-pass research are informed by [BedrockTools](https://github.com/QYCottage/BedrockTools); its full MIT notice is included below and in the packages. LeviLauncher and BedrockTools are used as development references. The Minecraft library is not distributed here.

## BedrockTools license

MIT License

Copyright (c) 2026 RadiantByte

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
