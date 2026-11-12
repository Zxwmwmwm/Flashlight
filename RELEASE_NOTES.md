# Flashlight 1.0.0-hf4

## Changes

- Widened the visible shaft's taper to make it read more clearly as a cone, including at smaller beam-size settings. It continues to extend from the aim toward the selected cone end.
- The hotfix suffix is now part of the manifest version string: `1.0.0-hf4`. There is no separate `hf` field.
- The `.levipack` contains only `manifest.json` and the native `.so`. The manifest no longer contains license notices; the required notices are included in the native library and in the repository.
- Sky shaft-only and soft-halo options, dark-world mode, cone positioning and saved controls remain available.

## Compatibility and limitations

The cone is a screen-space approximation rather than a volumetric light, and its appearance depends on screen position and graphics configuration. Sky looks depend on usable depth data; the panel reports when that data is unavailable. The light remains camera-relative in third person; player-position tracking is not implemented. See the README for compatibility details.
