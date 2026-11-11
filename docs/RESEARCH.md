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

The mathematical prototype, probe and CI are groundwork only, not a flashlight implementation.
