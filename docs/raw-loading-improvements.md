# RAW Loading Improvements

**Status:** Option 1 (embedded preview first) is implemented. Shader-based
editing and a GPU RAW pipeline are not implemented.

## Context

The app currently loads RAW files by decoding them with LibRaw on the CPU, then
uploads the processed RGB pixels to an OpenGL texture for display.

Current flow:

```text
Open RAW file
  -> start background load job
  -> LibRaw thumbnail extraction
  -> push preview result if available
  -> LibRaw full decode
  -> push full result
  -> main thread uploads ready result to OpenGL texture
  -> display texture with ImGui::Image
```

OpenGL is used for display, but the expensive RAW decode and processing work is
still CPU-based.

## Problem

Initial RAW loading can feel slow. Moving exposure and white balance controls to
a GPU shader will make editing responsive after the image is loaded, but it will
not automatically make the first full RAW decode faster.

The likely expensive step is LibRaw's full CPU processing, especially
`dcraw_process()`.

## Current Direction

The app uses a two-stage load pipeline:

```text
Open RAW
  -> quickly extract embedded preview JPEG
  -> upload preview texture
  -> show preview immediately
  -> continue full RAW decode in the background
  -> upload full 16-bit texture
  -> replace preview texture
```

This improves perceived loading speed without requiring a full GPU RAW pipeline
up front.

Important threading detail: do not store only one `std::future` and overwrite it
when a second file is opened. Destroying or replacing a still-running future
created by `std::async(std::launch::async, ...)` can block the UI thread until
that job completes. Keep outstanding futures in a collection, prune completed
jobs from the UI loop, and use generation IDs to discard stale image results.

## Implementation Options

### Option 1: Embedded Preview First

Use LibRaw to extract the embedded thumbnail or preview image first. Most RAW
files contain a JPEG preview that can be decoded much faster than the full RAW
sensor data.

Benefits:

- Fast visible feedback after opening a file.
- Keeps the current LibRaw + OpenGL architecture.
- Allows the full-quality decode to continue asynchronously.

Tradeoff:

- The first displayed image is not the final full-quality processed RAW.

### Option 2: Faster CPU Preview Decode

Keep using LibRaw for full decode, but configure a lower-quality or half-size
preview decode for quick display.

Benefits:

- Uses RAW data rather than embedded camera JPEG.
- Can be more representative of final rendering than a camera preview.

Tradeoff:

- More work than extracting the embedded preview.
- Still CPU-bound.

### Option 3: GPU RAW Pipeline Later

Use LibRaw mainly to unpack sensor data and metadata, then upload Bayer or
X-Trans data to the GPU and perform demosaic, black/white level adjustment,
white balance, color conversion, and tone mapping in shaders or compute shaders.

Benefits:

- Potentially faster interactive RAW processing.
- More control over the image pipeline.

Tradeoffs:

- Significant architecture change.
- Requires correct handling of camera metadata, sensor patterns, black levels,
  color matrices, and demosaic quality.
- Better deferred until the app has a clearer editing model.

## Implementation Status

- [x] Represent preview and full-resolution images separately.
- [x] Keep every background load future until it completes.
- [x] Extract and display an embedded preview when available.
- [x] Continue full RAW decode asynchronously.
- [x] Replace the preview texture with the full-resolution texture.
- [x] Reject stale results with generation IDs.
- [ ] Add a custom OpenGL shader path for viewer rendering.
- [ ] Move exposure, white balance, gamma correction, and tone mapping into the
  shader. The current controls do not alter the displayed image.

## Notes For Later

- Keep OpenGL calls on the main thread.
- Background threads may decode image data, but texture creation and deletion
  must stay on the thread that owns the OpenGL context.
- Use generation IDs so opening a second file does not display an older decode
  result.
- Keep outstanding loader futures alive until they complete. Replacing a running
  async future on the UI thread can block while LibRaw is still decoding.
- Avoid committing to a full GPU RAW pipeline until the simpler two-stage loader
  is working and measured.
