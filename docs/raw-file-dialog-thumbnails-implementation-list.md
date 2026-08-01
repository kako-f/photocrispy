# RAW File Dialog Thumbnails: Implementation List

## Current Status

Not implemented. The file dialog currently filters `.ARW`, `.DNG`, and `.RAF`
files but does not enable or register thumbnail support. The project does
already own the single `STB_IMAGE_IMPLEMENTATION` definition in
`StbImageDecoder.cpp`, and the existing viewer preview loader can decode
embedded JPEG or bitmap previews. `external/ImGuiFileDialog` has not been
modified for the generic RAW thumbnail hook described below.

## Goal

Show thumbnails for `.ARW`, `.DNG`, and `.RAF` files in ImGuiFileDialog by
extracting their embedded previews with LibRaw. Decode embedded JPEG data with
`stb_image`, resize it on the CPU, and create or delete OpenGL textures only on
the main thread.

Full RAW decoding must not be used for file-dialog thumbnails. If a file does
not contain a usable embedded preview, the dialog should display its normal
placeholder.

## Proposed Data Flow

```text
ImGuiFileDialog thumbnail worker
  -> identify regular image or RAW file
  -> regular image: decode with stb_image
  -> RAW file: extract embedded preview with LibRaw
       -> JPEG preview: decode from memory with stb_image
       -> bitmap preview: convert to RGB/RGBA pixels
  -> resize to the configured thumbnail bounds
  -> queue decoded RGBA8 pixels for upload

Main/render thread
  -> upload queued pixels to an OpenGL texture
  -> display texture in ImGuiFileDialog
  -> delete texture when the dialog releases the thumbnail
```

## Implementation Checklist

### 1. Establish stb_image Ownership

- [x] Ensure `STB_IMAGE_IMPLEMENTATION` is defined in exactly one translation
  unit.
- [x] Keep the project-owned implementation in `StbImageDecoder.cpp`, if
  practical.
- [ ] Configure ImGuiFileDialog not to define another implementation when
  thumbnails are enabled. Its current integration supports
  `DONT_DEFINE_AGAIN__STB_IMAGE_IMPLEMENTATION`.
- [ ] Check whether the stb resize implementation also needs a single project
  owner or can remain owned by ImGuiFileDialog.
- [ ] Build once at this stage to catch duplicate-symbol errors early.

### 2. Enable ImGuiFileDialog Thumbnail Support

- [ ] Enable `USE_THUMBNAILS` through the build configuration rather than
  editing the vendored configuration header, if supported cleanly.
- [ ] Confirm the thumbnail list view appears in the dialog.
- [ ] Initially verify it with a normal `.jpg` or `.png` file.
- [ ] Do not add RAW extensions to the existing `stbi_load(filename)` list and
  expect them to work; stb_image does not decode RAW containers.

### 3. Add a Generic CPU Thumbnail Loader Hook

The current ImGuiFileDialog version only exposes texture creation and
destruction callbacks. Those callbacks run after file data has been decoded, so
they cannot add RAW decoding by themselves.

- [ ] Add a generic thumbnail-loading callback to the local ImGuiFileDialog
  integration.
- [ ] Keep the callback format-agnostic; do not put PhotoCrispy-specific RAW
  behavior directly into the dialog.
- [ ] Pass the complete source path to the callback.
- [ ] Have the callback return decoded RGBA8 pixels, width, and height, or
  decline the file.
- [ ] When the callback declines a file, preserve ImGuiFileDialog's existing
  stb_image path for its supported image formats.
- [ ] Add RAW extensions to the dialog's eligible-thumbnail extension check so
  they are sent to the new callback.
- [ ] Document the small vendored patch so it can be reapplied or removed when
  upgrading ImGuiFileDialog.

Possible callback shape:

```cpp
using LoadThumbnailFun = std::function<bool(
    const std::string& path,
    std::vector<uint8_t>& rgbaPixels,
    int& width,
    int& height)>;
```

### 4. Add a Thumbnail-Specific RAW Loader

- [ ] Add a project-owned function such as:

  ```cpp
  std::optional<ImageData> extractEmbeddedRawThumbnail(
      const std::string& path,
      int maxWidth,
      int maxHeight);
  ```

- [ ] Share low-level embedded-preview extraction with
  `extractEmbeddedPreviewImage()` where useful.
- [ ] Keep viewer preview sizing and file-dialog thumbnail sizing separate.
- [ ] Use `LibRaw::open_file()`, `unpack_thumb()`, and
  `dcraw_make_mem_thumb()`.
- [ ] Decode `LIBRAW_IMAGE_JPEG` through the existing project-owned stb_image
  memory decoder.
- [ ] Convert `LIBRAW_IMAGE_BITMAP` previews with at least three channels to a
  consistent pixel representation.
- [ ] Always release successful LibRaw allocations with
  `LibRaw::dcraw_clear_mem()`.
- [ ] Return failure normally when an embedded preview is missing or invalid.
- [ ] Never fall back to `decodeFullRawImage()` for a dialog thumbnail.

### 5. Normalize and Resize CPU Pixels

- [ ] Normalize thumbnail output to RGBA8 before it reaches the OpenGL upload
  callback.
- [ ] Preserve the source aspect ratio.
- [ ] Resize immediately after preview extraction rather than retaining a
  multi-megapixel decoded camera preview in the dialog queue.
- [ ] Choose a configurable bound, initially around 128 or 160 pixels on the
  longest edge.
- [ ] Ensure the calculated width and height are never zero.
- [ ] Verify portrait and landscape previews.
- [ ] Investigate preview orientation metadata and rotate thumbnails if camera
  files are displayed incorrectly.

### 6. Register Main-Thread OpenGL Callbacks

- [ ] Register `SetCreateThumbnailCallback()` during application
  initialization.
- [ ] In the creation callback, create an RGBA8 OpenGL texture with linear
  filtering.
- [ ] Set `GL_UNPACK_ALIGNMENT` appropriately before uploading tightly packed
  pixels.
- [ ] Store the OpenGL texture ID in `IGFD_Thumbnail_Info::textureID` using a
  safe `uintptr_t` conversion.
- [ ] Mark the thumbnail ready for display after a successful upload.
- [ ] Release the CPU pixel buffer after upload using the allocation convention
  expected by ImGuiFileDialog.
- [ ] Register `SetDestroyThumbnailCallback()` and call `glDeleteTextures()`
  there.
- [ ] Clear the stored texture ID and display state after deletion.
- [ ] Call `ManageGPUThumbnails()` once per frame while the OpenGL context is
  current.
- [ ] Ensure no LibRaw worker or ImGuiFileDialog worker calls OpenGL directly.

### 7. Control Work and Memory Usage

- [ ] Confirm ImGuiFileDialog only queues thumbnails as they are needed by the
  visible thumbnail view.
- [ ] Avoid decoding the same file repeatedly while staying in one directory.
- [ ] Cache failed attempts for the current directory so files without embedded
  previews are not continually retried.
- [ ] Consider a modest maximum pending-job count for directories containing
  hundreds or thousands of RAW files.
- [ ] Ensure changing directories safely discards queued pixel buffers and
  deletes textures.
- [ ] Ensure closing the dialog drains or stops its thumbnail worker without
  leaving a thread using application state.

### 8. Error Handling

- [ ] Treat missing previews, unsupported preview types, corrupt JPEG data, and
  LibRaw errors as non-fatal thumbnail failures.
- [ ] Display the ordinary file icon or placeholder after failure.
- [ ] Avoid logging the same failure every frame.
- [ ] Add useful debug-only messages containing the file path and LibRaw error
  code.
- [ ] Make allocation and size calculations overflow-safe before copying or
  resizing pixel buffers.

### 9. Automated Tests

- [ ] Add unit tests for aspect-ratio-preserving thumbnail dimensions.
- [ ] Add tests for RGB-to-RGBA conversion.
- [ ] Add tests for bitmap previews containing three and four source channels.
- [ ] Add a test that malformed JPEG memory returns failure safely.
- [ ] Add a test that a missing embedded preview does not request a full RAW
  decode.
- [ ] Keep OpenGL texture creation out of ordinary unit tests unless a suitable
  test context is explicitly created.

### 10. Manual Verification

- [ ] Open a directory containing a mix of `.ARW`, `.DNG`, `.RAF`, `.jpg`, and
  non-image files.
- [ ] Verify thumbnails appear without selecting or fully opening each RAW.
- [ ] Test RAW files from multiple camera manufacturers.
- [ ] Test a RAW with no embedded preview and confirm browsing remains usable.
- [ ] Test corrupt and truncated files.
- [ ] Change directories repeatedly and watch for stale thumbnails.
- [ ] Close and reopen the dialog repeatedly.
- [ ] Browse a large RAW directory and check UI responsiveness and memory use.
- [ ] Open a RAW in the viewer while dialog thumbnail extraction is active.
- [ ] Confirm all OpenGL creation and deletion remains on the main thread.
- [ ] Run the normal build and tests with the platform's CMake preset (`linux`
  on Debian or `windows` from a Visual Studio Developer Command Prompt).

## Suggested Implementation Order

1. Resolve single-owner stb_image configuration.
2. Enable standard ImGuiFileDialog thumbnails and register OpenGL callbacks.
3. Verify ordinary JPEG/PNG thumbnails and texture cleanup.
4. Add the generic CPU thumbnail-loader callback to ImGuiFileDialog.
5. Add the project-owned LibRaw embedded-thumbnail loader.
6. Normalize, resize, and connect RAW output to the dialog callback.
7. Add failure caching and queue/memory limits if profiling shows they are
   needed.
8. Add automated coverage and perform manual testing with representative RAW
   files.

## Completion Criteria

- RAW thumbnails appear from embedded previews without invoking full RAW
  processing.
- Ordinary ImGuiFileDialog image thumbnails continue to work.
- Directory browsing stays responsive with a large set of RAW files.
- Missing or invalid previews degrade to a placeholder without repeated work.
- CPU decoding runs off the UI thread.
- OpenGL texture creation and deletion run only on the main thread.
- Closing the dialog or changing directories releases CPU buffers and GPU
  textures without stale results, crashes, or leaks.
