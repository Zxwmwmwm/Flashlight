# Flashlight

A customizable camera-following flashlight experiment for LeviLauncher on Android ARM64. Adjust the beam color, strength, size, softness, placement, ambient brightness, and the delay as it follows camera turns. Fast turns can carry the lagged beam out of view rather than snapping it back to the center.

## Experimental rendering test — 0.2.0 hotfix

This is a **screen-space GLES experiment**, not true dynamic world lighting. It changes the brightness and tint of existing world-render pixels; it cannot recover detail from completely black pixels, cast physical shadows, or light blocks outside the current view. The first device test found that the world callback has no current EGL context. The current implementation observes GLES clear operations on the render thread and uses a conservative pass-boundary heuristic to try to process the world before the HUD. The user reported that the beam appeared and worked on their device. UI isolation, lag behavior, and the new 0.2.0 controls still need further device validation. The test can affect the hand or other rendered elements depending on render order. Sky masking uses the depth buffer where available; it is an approximation, may misclassify distant geometry or sky pixels, and can fall back to an unmasked beam if depth copying is unsupported. Graphics modes using a different rendering backend may not work.

The test installs native camera, GLES and EGL hooks and checks for the exact GNU build ID `c60fa32908019eecae39ebfe7bac5755dc35cde2` of the supplied Android library, plus two expected signatures. Other builds are rejected even if their version labels look similar. These checks reduce the risk of a wrong-build hook; they do **not** prove the ABI, object layout, render pass, or hooks are safe. It starts with the beam **off**; the hooks are installed when the mod is enabled. In case of a crash or visual issues, disable the mod in LeviLauncher and restart Minecraft. Avoid combining the test with other rendering-hook mods during initial testing.

### Try the test

1. Disable the earlier Flashlight Probe and other experimental rendering mods. Import `Flashlight-Test.levipack` in LeviLauncher and enable it.
2. Enter a world. Tap the floating **FL** button and then **Turn beam on**. Move the camera and try the controls. Tap **Turn beam off** to restore the unprocessed view. The beam runs only when the launcher reports the gameplay HUD, not a menu.
3. Toggle **Dark world** to darken the surroundings. When on, use **Dark-world brightness** to choose how dark; when off, the surroundings retain their original brightness. The toggle is off by default, and the previous brightness value is kept.
4. Drag the panel by its title and resize it using the **↘ RESIZE** handle at the bottom. The panel position and size are saved and clamped to the available screen.
5. **Sky masking** is on by default. It attempts to keep the beam off far-background pixels. If depth copying is unavailable, the panel reports it and the beam continues without a mask. You can turn masking off if it hides the beam incorrectly.
6. To hide the controls outside the Minecraft pause menu, enable **Pause-only controls**. Pause the game to reveal the FL button again; the launcher must support pause-screen detection. If the detection API is unavailable, controls remain accessible.
7. Check whether the beam appears, whether it lags naturally in the correct direction, and whether the HUD, hand, menus, and buttons remain unaffected. The on-screen counter reports successfully submitted test frames, not proof that the beam is visible or UI-safe.
8. If it does not appear or anything looks unusual, filter LogLens for **`FlashlightTest`** and share the log along with the in-panel status. Turn the beam off or disable the mod and restart.

Settings, including dark-world mode, pause-only controls, and panel size and position, are saved on the device; the on/off state resets to off when the mod starts. The bubble can be dragged for the current session. Minecraft world or server state is not intentionally changed.

## Read-only diagnostic

The separate `Flashlight-Probe.levipack` scans candidate signatures and logs counts under `FlashlightProbe`; it does not install hooks or render effects. The user's probe log recorded one candidate for each of the five selected signatures. This confirms matches in readable executable memory, not function identity or safe behavior.

## Build

Requirements: Linux x86-64, Python 3, C++17, JDK, Android NDK r28c, Android platform `android.jar`, and a D8/R8 jar.

```sh
bash tests/run.sh
# Optional host-side software EGL/GLES test (libEGL and libGLES development headers required):
bash tests/run_render.sh
export ANDROID_NDK_HOME=/path/to/android-ndk-r28c
export ANDROID_JAR=/path/to/android.jar
export R8_JAR=/path/to/d8.jar
bash tools/build_test.sh
bash tools/build_probe.sh
```

Build outputs are under `artifacts/` and are not committed. GitHub Actions tests and builds both packages on pushes, pull requests, and manual dispatch. Tags named `test-v*` and `probe-v*` publish separate experimental releases, each with its respective `.levipack` as the sole uploaded asset. The Flashlight rendering-test version remains 0.2.0 and the hotfix manifest includes `"hf": true`.

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
