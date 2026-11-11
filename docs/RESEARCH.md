# Rendering research

Goal: a configurable view-following flashlight whose beam lags naturally as the camera turns, with the Minecraft UI unaffected. Distinguish screen-space relighting of existing pixels from actual world-surface illumination; neither visual quality nor exclusion of the UI has been demonstrated on a device.

## References and static findings

- BedrockTools commit `c03d3ae0c9022557de0acdf0d94e373e0b9e0579`, `src/modules/visual/motionblur.cpp`, demonstrates one GLES world post-processing strategy: observe color/depth clears to find a world pass, obtain camera orientation and render before later passes. This is a reference, not proof it works on the target build, on Vulkan, or in every graphics mode. Its heuristics and offsets need independent validation.
- The supplied stripped ARM64 `libminecraftpe.so` has SHA-256 `7db55a811e391624d0cba942ab0e438fdbc50071f8ec483a24b02ff49bc8267e`, GNU build ID `c60fa32908019eecae39ebfe7bac5755dc35cde2`, and contains `1.26.62.01_RC0`. No game binaries are included.
- File scanning against the five BedrockTools signature patterns produced one candidate each: RenderLevel `0xb7a38a0`, UpdatePlayerFromCamera `0x8a42820`, GetFov `0xb7c8e64`, ClientInstanceUpdate `0x9b7b338`, and ClientInstanceGetLocalPlayer `0x9b7f6d0`. These are **file offsets**, not runtime addresses, verified ABI, or verified function identities. Do not install hooks solely on this basis.
- The screenshot resembles a flashlight-style RenderDragon shader. Such shaders generally require a compatible shader loader; do not assume they work unmodified as a native mod or redistribute third-party shader assets without permission.

## Next steps

1. Collect on-device read-only candidate counts with the diagnostic probe. Identify the active graphics API and render mode independently.
2. Validate camera orientation, object layouts, render pass ordering, and lifecycle on the exact target build without trusting file offsets alone. Determine whether a safe pre-HUD world pass exists.
3. Prototype a reversible beam on that pass and distinguish simulated post-process exposure from actual geometric/dynamic light. Check black pixels, the hand, particles, menus and HDR; implement configurable settings once the renderer is proven.
4. Integrate the tested quaternion smoothing math and reset it on world transitions, discontinuities, and pauses. Validate on device for lag direction, frame rate and orientation conventions.

The initial mathematical prototype and probe were groundwork; see the experimental screen-space test below.

## Device probe and first screen-space test

The user ran the read-only probe and reported one candidate for each of the five patterns. This supports runtime pattern matching on that device, but it does not identify the graphics backend or validate the ABI and object offsets.

An opt-in native GLES test is now available separately as `Flashlight-Test.levipack`, still version 0.1.0. It installs guarded candidate `RenderLevel` and `UpdatePlayerFromCamera` hooks only if the exact supplied build ID and expected entry patterns match. The render callback requires a current EGL context, GLES 3, and a full-surface viewport; it copies the current color buffer and applies a multiplicative soft-edged beam. It does not use scene depth or add physical light, so black pixels stay black. Camera lag uses a smoothed quaternion and an approximate 70-degree vertical FOV; view conventions, render threading, HUD separation, and actual target framebuffer need device validation. Some graphics configurations, intermediate framebuffers, or MSAA arrangements may skip the pass or fail to copy.

The beam starts off. There is a separate Android control panel with saved color, brightness, radius, softness, delay, placement, and ambient settings. GLES resources are owned per rendering thread/context; they are retained until context destruction or context/surface change, not proactively deleted when a mod is toggled off. As with any native hook, disabling or unloading during a callback may be unsafe depending on the launcher hook implementation; restart the game after disabling the experiment. Do not run simultaneously with other mods hooking the same render/camera functions during the initial test.

The host math suite and a Mesa software EGL test validate selected calculations, shader compilation, visible bright-center/dark-edge output, off behavior, and GL state restoration for a controlled buffer. These tests are not Android device or Minecraft render-order validation.


## Android feedback and hotfix

The user reported the first test showed `No EGL context at world render callback; effect skipped`. This demonstrates that callback was not a valid GLES execution point on that device; it does not by itself establish the active graphics backend. They also supplied a screenshot showing the controls extending beyond the panel.

The 0.1.0 hotfix observes `glClear`, `glClearBufferfv`, `glClearBufferfi`, and `eglSwapBuffers` to look for a full-size color-clear followed by a depth-only clear on the same framebuffer, with a changed active program. This strategy is informed by BedrockTools motion blur and remains a heuristic: it can be skipped or choose the wrong pass on a different device or graphics mode. A launcher HUD/menu/pause state gate suppresses the effect outside reported gameplay. Counts are logged periodically under `FlashlightTest` when swapping while the effect is on. Camera, GPU, and UI behavior still need device testing.

Controls were redesigned with a fixed header, a clipped scrolling body, and dimensions calculated from the actual parent view instead of display metrics. An optional persistent pause-only setting uses the launcher `PreloaderInput` pause state. If that API cannot be called, controls stay visible to avoid locking the user out; unknown screen state also suppresses the experimental beam. Software EGL and pure host tests do not prove Minecraft render order or Android layout behavior.


## 0.2.0 feedback and controls

The user reported that the 0.1.0 hotfix beam appears and works. They liked the dark surroundings and requested that it become optional, along with a movable, resizable panel. Dark-world mode is now off by default; when off, the shader ambient multiplier is 1, and when on it uses the saved dark-world brightness. The existing camera and GLES hook strategy is unchanged. The panel saves its bounds as normalized screen fractions and clamps them to the current overlay size. Automated geometry, pause-policy, ambient, and software EGL checks do not replace device testing of the new controls.
