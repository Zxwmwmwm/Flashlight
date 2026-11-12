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


## 0.2.0 hotfix: fast turns and sky

The user observed a snap on fast turns and a beam in the sky. The prior center projection reset to the screen center once the delayed direction passed behind the camera. The beam now compares each screen ray to the delayed direction in camera space, so fast turns can carry it smoothly out of view and back without that reset. It still uses an approximate field of view.

The GLES path attempts to copy the current depth buffer, then masks the beam at far-clear-depth pixels. This is screen-space masking, not physical lighting: the result depends on the depth attachment, depth convention, render pass and clear value. It cannot identify every kind of sky or distant surface, recover texture detail lost to darkness, cast shadows or create a true volumetric cone. If depth cannot be copied, the panel reports that sky masking is unavailable and the beam falls back unmasked. A persisted Sky masking toggle can disable it if the estimate is incorrect. Software EGL tests cover forward and reverse clear-depth, near depth, fallback, extreme turns and GL texture-state restoration; Android and actual game depth contents need verification.


## 0.3.0: optional visible light cone

The user reported the 0.2.0 hotfix was working and requested a switchable visible cone resembling a reference image. The new persisted Visible cone setting defaults off. It adds a tapered, soft screen-space shaft between a lower-right origin and the delayed beam direction, using the same color, strength, beam size and sky mask. It does not introduce a held flashlight object. The shaft is a stylized post-process, not true participating-media rendering: it cannot trace a 3D volume or reliably determine occlusion, and sky masking depends on a usable and representative depth buffer. Mesa software EGL tests exercise cone on/off, pixels outside the shaft, sky masking and large camera turns; visual and performance validation on Android remains necessary.


## 1.0.0: cone origin controls

Two saved screen-space controls independently set the shaft origin from left to right and top to bottom, defaulting to the previous lower-right position. Existing beam-position controls still adjust the spotlight aim separately. Software EGL tests cover changes to both axes. These controls and their appearance require real-device validation; the same screen-space and depth-buffer limitations continue to apply.


## 1.0.0 hotfix: visible sky shaft

The user requested a shaft extending into open sky or long distances while keeping sky spotlight masking. The surface spotlight remains multiplied by the depth mask. When Visible cone is enabled, the stylized shaft is independent of that mask; at far depth it extends beyond the aim point. If depth is unavailable or masking is disabled, the extended profile is used everywhere. This is a screen-space approximation rather than physical volumetric lighting and can draw over intervening geometry. Host EGL tests cover conventional and reverse depth, sky spotlight masking, shaft extension, cone off, missing depth and masking off. Android visual results and occlusion remain device-dependent.


## 1.0.0 hotfix 2: cone aim and package contents

The user reported an incorrect shaft after repositioning the cone. The sky extension had been selected independently for each pixel, so sky behind a surface could extend the shaft past the actual spotlight. The shader now samples depth at the projected aim to decide whether the whole shaft extends; offscreen and missing-depth cases retain the extended profile. Software EGL tests include a mixed surface/sky scene, sky-only conventional and reverse depth, and the unavailable-depth fallback.

The user also requested that the light follow the player in third person. The current hook supplies camera orientation only. Reliable player-anchored rendering would require validated local-player identity, player/camera positions, projection and view mode; candidate offsets in BedrockTools are not sufficient evidence of a safe ABI on this exact binary. This hotfix does not claim or implement player anchoring. It keeps the existing camera-relative behavior.

Packaging now generates only manifest and native library, embedding complete Flashlight and BedrockTools MIT notices in a manifest field. The launcher source copies unknown manifest fields while normalizing; actual installation remains to be verified.


## 1.0.0 hotfix 3: sky looks and cone direction

The user requested a sky shaft without a bright center and a soft-halo alternative, both selectable, and asked that the cone extend from the aim toward the selected position. The persisted mutually exclusive sky looks use depth independently of the sky mask: shaft-only removes the sky spot and enables a sky shaft; halo replaces the sky spot with an edge-weighted glow. Surfaces keep the original spotlight. When depth is unavailable the panel reports it and the modes revert to the ordinary spotlight; there is no reliable way to identify the sky in that fallback. The cone no longer extends past the aim opposite the chosen end. Software EGL tests cover sky and surface behavior, modes, both depth conventions, position changes and fallback. Device appearance and actual depth semantics remain to be verified. Third-person player anchoring remains unimplemented.
