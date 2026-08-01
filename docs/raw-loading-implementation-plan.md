# RAW Loading Implementation Plan

**Status:** Implemented in the current code. The Debian build and automated
tests pass; Windows and real-camera RAW verification remain.

## Current State

The app already decodes RAW files on a background task and uploads OpenGL
textures on the main thread.

Current flow:

```text
File > Open
  -> increment m_loadGeneration
  -> start std::async preview/full load job
  -> worker tries LibRaw thumbnail extraction
  -> worker pushes preview result if available
  -> worker continues full LibRaw decode
  -> worker pushes full result if available
  -> worker pushes generation completion
  -> main thread polls queues and uploads accepted textures
```

This keeps OpenGL work on the main thread and allows the user to see an
embedded preview before full LibRaw processing completes.

## Implemented Shape

Split loading into a preview result, a full-resolution result, and a completion
notification.

```text
Open RAW
  -> start background load job
  -> extract embedded preview
  -> push preview result
  -> continue full RAW decode
  -> push full result
  -> push completed generation
  -> main thread uploads ready results
  -> preview is replaced by full image when available
```

OpenGL texture creation, replacement, and deletion must remain on the main
thread that owns the OpenGL context.

The current implementation uses:

- `m_loadFutures`: stores all outstanding `std::future<void>` loader jobs.
- `m_loadResults`: receives preview and full image results.
- `m_completedLoads`: receives the generation ID when a worker has finished
  preview and full decode attempts.
- `m_loadGeneration`: identifies the currently requested file and filters stale
  results.

## Image Data Types

Replace the single full-RAW-only data path with a format-aware image payload.

Possible shape:

```cpp
enum class ImageKind {
    Preview,
    Full
};

struct ImageData {
    std::vector<uint8_t> pixels8;
    std::vector<uint16_t> pixels16;
    int width = 0;
    int height = 0;
    int channels = 3;
    bool is16Bit = false;
    ImageKind kind = ImageKind::Full;
};
```

Preview images will usually be 8-bit RGB or RGBA. Full RAW output should stay
as 16-bit RGB.

An alternative is to keep `RawImageData` for full-resolution data and add a
separate `PreviewImageData`, but one payload type keeps texture upload simpler.

## Background Results

The old `std::future<std::optional<RawImageData>>` only produced one final
value. Preview-first loading needs multiple results.

Recommended approach:

- Start one background job per opened file.
- Let the job push `Preview` and `Full` results into a thread-safe queue.
- Let the job push a completed generation into a second queue when both preview
  and full attempts have finished.
- Store outstanding `std::future<void>` values in a collection and prune
  completed futures from the UI loop.
- Poll that queue from the UI loop.
- Upload textures only from the main thread.

Each result should carry a generation ID.

```text
m_loadGeneration increments on every file open.
Worker captures the current generation.
Main thread discards any result whose generation does not match.
```

This prevents an old decode from replacing a newer image if the user opens a
second file before the first decode finishes.

Do not overwrite a single running async future when a second file is opened.
Destroying or assigning over a still-running `std::future` created by
`std::async(std::launch::async, ...)` can block the UI thread until the old
decode finishes. Keeping futures in `m_loadFutures` avoids that stall while
older jobs finish in the background.

## Preview Extraction

Use LibRaw thumbnail APIs before full processing:

```text
LibRaw raw
  -> open_file(path)
  -> unpack_thumb()
  -> dcraw_make_mem_thumb()
```

Handle at least these thumbnail formats:

- `LIBRAW_IMAGE_JPEG`: embedded camera JPEG preview
- `LIBRAW_IMAGE_BITMAP`: already decoded bitmap-like preview

JPEG thumbnails need a JPEG decoder. Options:

1. Use `stb_image` from the existing ImGuiFileDialog vendored copy.
2. Add a project-owned `stb_image` wrapper.
3. Add a clean image decoding dependency through vcpkg.

Avoid modifying vendored dependencies under `external/`.

## UI Behavior

Recommended behavior on file open:

1. Keep the old image visible until a preview arrives, while showing a loading
   indicator.
2. When the preview arrives, upload and display the preview texture.
3. Keep indicating that the full RAW is still loading.
4. When the full image arrives, upload the full texture and delete the preview
   texture.
5. When the worker reports completion for the current generation, stop the
   loading state even if full decode failed.
6. Mark the load as complete.

Simpler alternative:

1. Clear the old image immediately.
2. Show a spinner until preview or full image data is ready.

The preview path should be optional. If preview extraction fails, continue the
full RAW decode and keep the loading state visible.

## Texture Upload

Make texture upload format-aware.

```text
8-bit preview -> GL_RGB8 or GL_RGBA8, GL_UNSIGNED_BYTE
16-bit full   -> GL_RGB16, GL_UNSIGNED_SHORT
```

The displayed texture object should track whether it represents a preview or
the full image so the UI can show an accurate loading state.

## Cancellation Strategy

Do not attempt hard cancellation in the first version. LibRaw full processing
cannot be cheaply interrupted once `dcraw_process()` is running.

Use generation IDs instead:

```text
Open file A -> generation 1
Open file B -> generation 2
File A finishes later -> discard result because generation 1 is stale
File B result arrives -> accept result because generation 2 is current
```

A persistent worker thread and stronger cancellation can be added later if
needed.

## Shader Work

Do not bundle shader rendering into the first preview-loading implementation.

Recommended phases:

1. Preview-first loading with full image replacement.
2. Texture/render cleanup and status UI.
3. Custom OpenGL shader path for viewer rendering.
4. Move exposure, white balance, gamma, and tone mapping into the shader.

Keeping these separate makes the threading and state changes easier to test.

## Phase 1 Status

- [x] Add preview/full image data types.
- [x] Add embedded preview extraction using LibRaw thumbnail APIs.
- [x] Add JPEG thumbnail decode support.
- [x] Add a thread-safe load result queue with generation IDs.
- [x] Change `File > Open` to start a load job that emits preview then full.
- [x] Store load futures in `m_loadFutures` instead of overwriting one future.
- [x] Add a completed-load queue so failed full decodes can end the loading
  state.
- [x] Poll pending load results and upload textures on the main thread.
- [x] Replace the preview texture with the full texture when ready.
- [x] Build and run the automated tests on Debian with the `linux` preset.
- [ ] Build and run the automated tests on Windows with the `windows` preset.
- [ ] Manually test RAW files with and without embedded previews.
- [ ] Manually test opening file B while file A is still decoding; the UI must
  not hang, and file A must not replace file B.

Fallback behavior: if preview extraction fails, keep showing the spinner and
continue full decode. The app should not fail to open a file just because the
preview is missing.
