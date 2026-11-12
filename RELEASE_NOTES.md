# Flashlight 1.0.0 hotfix 2

## Changes

- The visible cone now checks the depth at the spotlight's aim when deciding whether to extend into the sky. When the aim lands on a surface, the shaft stops near the spotlight rather than continuing into background sky behind it. Move the cone's origin with the horizontal and vertical origin sliders.
- The `.levipack` contains only `manifest.json` and the native `.so`. Required MIT notices are included inside the manifest.
- The manifest remains at version `1.0.0` with `hf: true`.

## Current limitations

The light is still camera-relative in third person; player-position tracking is not yet implemented. Sky detection and the cone use screen-space approximations: their appearance depends on the game's depth buffer, and they do not create physically accurate occlusion or world lighting. If depth is unavailable, the shaft uses the extended profile. Compatibility is restricted to the specified Minecraft build; see the README for details.
