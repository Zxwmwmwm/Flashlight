# Flashlight 1.0.0 hotfix 3

## Changes

- Added **Sky shaft only** and **Sky soft halo** controls. The first replaces the bright sky spotlight with a tapered shaft; the second replaces it with a softer ring around the aim. They are mutually exclusive, saved on the device, and off by default. Tap an active option again to return to the previous sky-mask behavior.
- The visible cone now extends from the spotlight's aim toward the chosen **Cone horizontal end** and **Cone vertical end**, instead of projecting past the aim on the opposite side.
- The sky looks use depth data even when **Sky masking** is off. They take precedence over the sky mask while active; surfaces keep the normal spotlight.
- The package remains version `1.0.0` with `hf: true`, and contains only `manifest.json` and the native `.so`. MIT notices are embedded in the manifest.

## Compatibility and limitations

Sky detection depends on a usable depth buffer and may misidentify distant surfaces. If depth copying is unavailable, the panel reports **sky depth unavailable** and the new sky looks fall back to the ordinary spotlight rather than hiding the world. These are screen-space approximations, not volumetric lighting or physical occlusion. The light remains camera-relative in third person; player-position tracking is not implemented. See the README for compatibility details.
