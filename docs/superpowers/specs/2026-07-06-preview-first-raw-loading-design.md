# Preview-First RAW Loading Design

**Status:** Implemented. The Debian build and automated tests pass; Windows and
manual RAW-file verification are still required.

## Goal

Improve perceived RAW file loading speed by showing an embedded preview as soon
as it is available, then replacing it with the full 16-bit RAW decode when the
background load finishes.

This pass intentionally excludes shader-based exposure, white balance, gamma,
and tone mapping work.

## Current State

`App` starts a background job for each opened file and retains all outstanding
futures. Each worker attempts embedded-preview extraction, performs the full
LibRaw decode, and emits generation-tagged results. The main thread uploads the
accepted result and replaces the preview with the full 16-bit texture.

## Architecture

The implementation uses background load jobs that can emit
multiple results:

1. Try to extract an embedded preview with LibRaw thumbnail APIs.
2. Push a preview image result if extraction succeeds.
3. Continue the full RAW decode using the existing LibRaw path.
4. Push a full-resolution image result when complete.
5. Push the completed generation after preview and full attempts finish.

`App` owns a thread-safe queue of pending load results and a second queue of
completed load generations. `photoViewer()` polls those queues each frame,
discards stale generations, and uploads accepted images to OpenGL textures on
the main thread.

`App` keeps outstanding `std::future<void>` values in `m_loadFutures` and prunes
completed futures from the UI loop. This avoids a UI stall when opening file B
while file A is still decoding. Overwriting a single running `std::future`
created by `std::async(std::launch::async, ...)` can block until the previous
job completes.

## Data Model

The image kind and format-aware payload are:

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

struct LoadResult {
    uint64_t generation = 0;
    ImageData image;
};
```

Preview images are expected to be 8-bit RGB or RGBA. Full RAW images remain
16-bit RGB.

## Loading Behavior

Opening a file increments `m_loadGeneration`, sets loading state, and starts a
background job that captures the file path and generation. The future for that
job is appended to `m_loadFutures`; it is not assigned over an older running
future.

The job attempts embedded preview extraction first. If that fails, it silently
continues to full decode. Missing previews must not prevent the file from
loading.

The UI keeps the old image visible until a preview or full image arrives. While
the accepted image is a preview, the UI still indicates that the full image is
loading. When a full image arrives, the app replaces the current texture and
marks loading complete.

Generation IDs prevent stale background results from replacing a newer image:
if the user opens file B while file A is still decoding, later results from file
A are discarded by the main thread.

Completed-generation messages stop the loading state for the current generation
when both preview and full attempts have finished, including the case where a
preview was shown but full decode failed.

## Texture Upload

Texture upload is format-aware:

```text
8-bit RGB preview  -> GL_RGB8,  GL_RGB,  GL_UNSIGNED_BYTE
8-bit RGBA preview -> GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE
16-bit full RGB    -> GL_RGB16, GL_RGB,  GL_UNSIGNED_SHORT
```

Texture creation, replacement, and deletion remain on the main thread that owns
the OpenGL context.

## Preview Decoding

Use LibRaw thumbnail APIs:

```text
open_file(path)
unpack_thumb()
dcraw_make_mem_thumb()
```

Handle `LIBRAW_IMAGE_BITMAP` directly when LibRaw returns bitmap thumbnail data.
Handle `LIBRAW_IMAGE_JPEG` by decoding the embedded JPEG through a project-owned
decoder path. The first implementation should prefer a small, local `stb_image`
wrapper or a vcpkg image dependency. Do not modify vendored files under
`external/`.

## Error Handling

Preview extraction failure is non-fatal. Full decode failure leaves loading
complete with the previous image still visible if no accepted replacement was
uploaded.

The loader should clean up LibRaw memory with `LibRaw::dcraw_clear_mem()` for
every successful `dcraw_make_mem_*()` allocation.

## Testing And Verification

Add focused tests for non-OpenGL behavior where practical:

- Thread-safe queue push/pop ordering.
- Generation filtering behavior.
- Image payload format decisions if implemented outside direct OpenGL calls.

Manual verification requires RAW files with and without embedded previews:

- A RAW with an embedded preview should show an image before the full decode
  completes.
- A RAW without an embedded preview should keep the loading indicator and still
  load the full image.
- Opening a second RAW before the first finishes should never show the first
  image after the second open starts.
- Opening a second RAW before the first finishes should not make the UI hang
  while the first full decode continues.

Build verification uses the platform presets. `VCPKG_ROOT` must point to the
vcpkg installation.

Debian:

```sh
cmake --preset linux
cmake --build --preset linux
ctest --preset linux
```

Windows (from a Visual Studio Developer Command Prompt so MSVC is on `PATH`):

```powershell
cmake --preset windows
cmake --build --preset windows
ctest --preset windows
```
