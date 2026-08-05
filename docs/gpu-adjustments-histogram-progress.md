# GPU Adjustments and Histogram Progress

Last updated: 2026-08-05

## Goal

Display the loaded RAW through GPU-based exposure and RGB white-balance
adjustments, then compute a Lightroom/darktable-style RGB histogram from the
adjusted image whenever the source image or an adjustment changes.

## Current Progress

The first adjustment stage is implemented locally but remains uncommitted.

- Added `shaders/imageProcessing.vert`, which generates a fullscreen triangle
  from `gl_VertexID`.
- Added `shaders/imageProcessing.frag`, which applies:

  ```glsl
  adjusted = source.rgb * exp2(exposure) * whiteBalance;
  ```

- Added processing state and lifecycle methods to `App`:
  `initImageProcessing()`, `resizeProcessedImage()`, `processImage()`, and
  `destroyImageProcessing()`.
- The decoded source texture remains unchanged.
- Adjustments render into an `GL_RGBA8` framebuffer texture at the source image
  dimensions.
- Processing is marked dirty when a preview/full image arrives or an adjustment
  changes, rather than running continuously.
- The Viewer displays the processed texture and falls back to the decoded source
  texture if processing initialization fails.
- Exposure remains a `-5` to `+5` stop slider.
- White balance is an RGB multiplier with a `0` to `2` range.
- `Reset Adjustments` restores exposure to `0` and RGB to `(1, 1, 1)`.
- Shader program IDs now initialize to zero for safe cleanup.

The user reports that the project builds and the adjustments look correct by
visual inspection. No automated GPU test was added.

## Before Continuing

Perform these manual checks:

1. Exposure `+1` visibly brightens and `-1` darkens the image.
2. RGB `(0, 1, 1)` removes the red contribution.
3. Reset restores exposure `0` and RGB `(1, 1, 1)`.
4. Opening differently sized RAW files reallocates the processed texture safely.
5. Opening another RAW during full decode still rejects stale results.
6. Image orientation matches the previous direct-texture viewer.

Run the normal build checks again before committing:

```bash
cmake --preset linux
cmake --build --preset linux
ctest --preset linux
```

Use the equivalent `windows` presets from a Visual Studio Developer Command
Prompt when working on Windows.

## Small Cleanup Still Pending

- Remove the redundant `#include "graphics/shaderProgram.h"` from `src/App.cpp`;
  `App.h` already includes it.
- Remove the `// TODO = NECESARRY?` comment in `resizeProcessedImage()`.
- Remove the old commented-out direct `ImGui::Image` call in `photoViewer()`.
- Review comments and spelling without changing behavior.

## Histogram: Next Stage

The histogram should represent `m_processedTexture`, so it reflects both the
decoded RGB image and current GUI adjustments.

Recommended minimal implementation:

1. Add a 256-bin RGB compute shader using OpenGL 4.3 shader-storage buffers and
   atomic increments.
2. Clear the three bin arrays and dispatch only when image processing completes,
   not every frame.
3. Initially sample a reduced pixel set for responsive updates on large RAWs.
4. Issue the required memory barrier, then read back only the 768 bin counts.
5. Normalize the counts and draw three overlapping RGB curves with ImGui.
6. Keep all OpenGL buffer creation, dispatch, readback, and deletion on the main
   thread.
7. Retain generation handling indirectly by computing only after the accepted
   source result has produced `m_processedTexture`.

Before implementing the histogram, decide whether the curve normalization should
use the absolute maximum bin or a clipped percentile. A clipped percentile is
more Lightroom-like because a large black or white spike does not flatten the
rest of the graph, but absolute maximum is the simpler first version.

## Working Tree Warning

The repository also contains pre-existing, in-progress triangle renderer and
shader filename changes. Do not discard or overwrite them. Check `git status`
and stage only intentional files before any future commit.
