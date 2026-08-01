# Filmstrip Implementation Plan

## Current Status

The current code contains an early navigation skeleton, not the filmstrip
described by this plan:

- `FileBrowser` scans one directory for regular RAW files with case-insensitive
  `.arw`, `.dng`, `.cr2`, `.cr3`, `.nef`, `.raf`, `.rw2`, and `.orf`
  extensions.
- `App::filmStrip()` opens an ImGui `Filmstrip` window and draws selectable
  filenames.
- The browser refreshes on every frame after a directory has been chosen.
- A selection is stored by `FileBrowser`, but it is not connected to the viewer.
- Files are not sorted, directory errors are not handled, and no thumbnail,
  navigation, generation, worker-pool, or texture-lifetime model exists yet.
- Opening a file does not explicitly initialize the browser selection to that
  file.

## Goal

Add a persistent, horizontally scrollable filmstrip similar to Lightroom. When
the user opens an image, PhotoCrispy scans its parent directory, displays the
supported images as filmstrip items, and allows the user to move between them
without reopening the file dialog.

The selected image continues to use the existing preview-first RAW loading
pipeline. Filmstrip thumbnails use embedded RAW previews only and must never
trigger full RAW processing.

## Initial Scope

The first version should support:

- `.ARW`, `.DNG`, and `.RAF` files.
- Using the opened image's parent directory as the current directory.
- A horizontally scrollable row docked below the main viewer.
- Selection by clicking a filmstrip item.
- Previous and next navigation with the keyboard.
- Embedded-preview thumbnails loaded asynchronously.
- Placeholders for missing, failed, or not-yet-loaded thumbnails.
- Safe handling of rapid selection and directory changes.

Defer ratings, flags, range selection, drag and drop, metadata filters, file
watching, and collection management until the basic navigation workflow is
stable.

## User Experience

```text
+----------------------------------------------------------+
| Menu                                                     |
+--------------+-------------------------+-----------------+
| Navigation   |                         | Develop Settings|
|              |       Main Viewer       |                 |
|              |                         |                 |
+--------------+-------------------------+-----------------+
| <  [thumb] [thumb] [selected] [thumb] [thumb] [thumb]  > |
+----------------------------------------------------------+
```

Opening a file should:

1. Select and begin loading that file in the main viewer.
2. Treat its parent directory as the active filmstrip directory.
3. Scan the directory for supported image files.
4. Locate and highlight the opened file in the resulting list.
5. Scroll the selected filmstrip item into view.
6. Begin thumbnail work for visible and nearby items.

Clicking another item should immediately start the existing selected-image
loading flow. The embedded viewer preview should appear first, followed by the
full 16-bit decode.

## Architecture

Keep the selected-image loader and filmstrip-thumbnail loader separate:

```text
Selected image
  -> extract a viewer-sized embedded preview
  -> display preview
  -> continue full 16-bit RAW decode
  -> replace preview with full image

Filmstrip item
  -> extract embedded preview
  -> decode embedded JPEG or copy bitmap data
  -> resize to a small RGBA8 thumbnail
  -> stop; never perform full RAW processing
```

The two paths may share low-level embedded-preview extraction and resizing
utilities, but they should not share job state, texture ownership, or result
queues.

## Data Model

Introduce a model outside the rendering code so folder state, selection, and
thumbnail state can be tested independently.

Possible thumbnail states:

```cpp
enum class ThumbnailState
{
    NotRequested,
    Queued,
    Decoding,
    ReadyForUpload,
    Ready,
    Failed
};
```

Possible filmstrip item:

```cpp
struct FilmstripItem
{
    std::filesystem::path path;
    ThumbnailState thumbnailState = ThumbnailState::NotRequested;

    unsigned int textureId = 0;
    int thumbnailWidth = 0;
    int thumbnailHeight = 0;
};
```

Possible model responsibilities:

```cpp
class FilmstripModel
{
public:
    void openDirectory(const std::filesystem::path& directory);
    void selectFile(const std::filesystem::path& path);

    std::vector<FilmstripItem> items;
    std::size_t selectedIndex = 0;
    uint64_t directoryGeneration = 0;
};
```

Prefer a single selected index in the model instead of duplicating a `selected`
boolean across every item.

## Threading and Ownership Rules

Worker threads may:

- Open RAW files through LibRaw.
- Extract embedded previews.
- Decode embedded JPEG data through stb_image.
- Convert bitmap previews to a consistent pixel format.
- Resize previews to thumbnail dimensions.
- Push completed CPU data into a thread-safe queue.

Only the main thread may:

- Call `glGenTextures()`.
- Call `glTexImage2D()`.
- Call `glDeleteTextures()`.
- Assign texture IDs to filmstrip items.
- Replace the selected viewer texture.

Do not launch an unbounded number of `std::async` operations. Use a bounded
thumbnail job queue serviced by a small fixed number of workers, initially one
or two.

The existing selected-image futures must continue to be retained until they
complete. Never overwrite or destroy a still-running future in a way that can
block the UI thread.

## Generations and Stale Results

Maintain two independent generations:

- The existing selected-image load generation rejects late preview and full
  results after another image is selected.
- A filmstrip directory generation rejects late thumbnails after another
  directory is opened.

Possible thumbnail result:

```cpp
struct ThumbnailResult
{
    uint64_t directoryGeneration = 0;
    std::filesystem::path path;
    std::vector<uint8_t> rgbaPixels;
    int width = 0;
    int height = 0;
};
```

Before uploading a result, the main thread must verify:

1. Its directory generation still matches.
2. The path still exists in the active filmstrip model.
3. The corresponding item still expects that result.

## Implementation Checklist

### 1. Add Folder Scanning

- [x] Add a supported-extension helper with case-insensitive matching.
- [x] Scan the last file-dialog directory using `std::filesystem`.
- [x] Include regular files only.
- [x] Support `.ARW`, `.DNG`, and `.RAF` (plus common RAW extensions).
- [ ] Decide whether JPEG, PNG, and TIFF should be added in the same change or
  in a follow-up.
- [ ] Apply natural filename sorting so numeric filename sequences appear in
  intuitive order.
- [ ] Locate the originally opened file after sorting.
- [ ] Handle missing, inaccessible, and empty directories without crashing.
- [ ] Avoid following recursive subdirectories in the first version.

### 2. Add the Filmstrip Model

- [ ] Add `FilmstripItem` and `ThumbnailState` types.
- [ ] Add a model that owns the current directory, item list, selected index,
  and directory generation.
- [ ] Expose selection by index and by normalized path.
- [ ] Ensure a new directory increments the generation before clearing old
  state.
- [ ] Add helpers for previous and next selection.
- [ ] Define whether navigation wraps at the first and last items. Initially,
  clamping is less surprising than wrapping.
- [ ] Keep OpenGL deletion outside the model unless the model is explicitly
  given a main-thread texture-release callback.

### 3. Render a Placeholder-Only Filmstrip

- [x] Add a `Filmstrip` ImGui window.
- [ ] Render it as a horizontal `BeginChild()` region with a horizontal
  scrollbar.
- [ ] Give every item a stable ImGui ID based on its path or stable item ID.
- [ ] Render a fixed-size placeholder for each item.
- [ ] Render a shortened filename beneath each placeholder.
- [ ] Show the complete filename in a tooltip.
- [ ] Draw a clear border or background around the selected item.
- [ ] Keep item geometry consistent for portrait and landscape images.
- [ ] Make the filmstrip height resizable through docking where practical.

This milestone should work before thumbnail decoding is introduced.

### 4. Connect Selection to the Viewer

- [ ] When an item is clicked, update the selected index.
- [ ] Start the existing preview-first selected-image load.
- [ ] Increment the selected-image generation.
- [ ] Preserve all outstanding viewer futures until completion.
- [ ] Reset or retain viewer zoom according to an explicit policy. Resetting to
  fit on image change is a reasonable initial behavior.
- [ ] Do not rescan the directory when moving between items already in the
  active filmstrip.
- [ ] Ensure rapid clicking never shows a stale preview or full image.

### 5. Add Keyboard Navigation

- [ ] Support left and right arrows for previous and next images when focus is
  not captured by another editing control.
- [ ] Consider `Home` and `End` for first and last image.
- [ ] Ensure keyboard navigation uses the same selection function as mouse
  clicks.
- [ ] Automatically scroll the selected thumbnail into view.
- [ ] Avoid stealing arrow keys while an active slider, text field, or other
  widget needs them.

### 6. Add the Thumbnail Worker Queue

- [ ] Define a thumbnail job containing directory generation, path, requested
  bounds, and stable item identity if needed.
- [ ] Add a thread-safe pending-job queue.
- [ ] Add a thread-safe completed-result queue.
- [ ] Start a fixed worker pool during application initialization.
- [ ] Stop and join workers during application shutdown.
- [ ] Avoid queueing duplicate work for an item already queued, decoding,
  ready, or failed.
- [ ] Ensure changing directories invalidates old jobs without requiring hard
  cancellation inside LibRaw.
- [ ] Allow workers to finish stale LibRaw calls, then discard their results by
  generation.

### 7. Extract and Resize RAW Thumbnails

- [ ] Reuse or factor the existing LibRaw embedded-preview extraction logic.
- [ ] Use `open_file()`, `unpack_thumb()`, and `dcraw_make_mem_thumb()`.
- [ ] Decode `LIBRAW_IMAGE_JPEG` from memory through the project-owned
  stb_image wrapper.
- [ ] Convert `LIBRAW_IMAGE_BITMAP` previews containing at least three channels.
- [ ] Release every successful LibRaw memory allocation with
  `LibRaw::dcraw_clear_mem()`.
- [ ] Normalize final thumbnail pixels to RGBA8.
- [ ] Resize in the worker immediately after extraction.
- [ ] Preserve aspect ratio within a bound of approximately 128 to 160 pixels
  on the longest edge.
- [ ] Ensure calculated dimensions are at least one pixel.
- [ ] Investigate orientation metadata if portrait thumbnails appear rotated.
- [ ] Mark missing or invalid embedded previews as failed.
- [ ] Never fall back to a full RAW decode for a filmstrip thumbnail.

### 8. Schedule Visible and Nearby Items

- [ ] Determine which items intersect the visible horizontal region.
- [ ] Queue visible items first.
- [ ] Queue a small prefetch range on both sides of the viewport.
- [ ] Give the selected item the highest thumbnail priority.
- [ ] Avoid immediately decoding every image in a large directory.
- [ ] Reprioritize pending work after large scroll or selection jumps if the
  queue design supports it.
- [ ] Keep thumbnail work from saturating the CPU or disk while the selected
  image is performing its full decode.

An initial scheduling policy can be:

```text
priority 1: selected item
priority 2: visible items
priority 3: 8 to 16 items before and after the visible range
priority 4: no background work for the rest of the directory
```

### 9. Upload and Release Thumbnail Textures

- [ ] Drain completed thumbnail results from the main/render thread.
- [ ] Reject results with a stale directory generation.
- [ ] Upload valid results as RGBA8 OpenGL textures.
- [ ] Use linear minification and magnification filtering.
- [ ] Set the appropriate pixel unpack alignment.
- [ ] Assign texture dimensions and state to the matching filmstrip item.
- [ ] Release CPU pixels after upload.
- [ ] Delete every filmstrip texture when changing directories.
- [ ] Delete every filmstrip texture during application shutdown.
- [ ] Ensure replacing a thumbnail cannot leak its prior texture.
- [ ] Display a placeholder while queued or decoding and a distinct fallback
  after permanent failure.

### 10. Add Memory Controls

- [ ] Resize embedded previews before placing them in the completed queue.
- [ ] Bound the number of pending jobs and completed CPU results.
- [ ] Measure memory consumption in a directory containing at least 1,000 RAW
  files.
- [ ] Initially consider retaining all completed GPU thumbnails for the active
  directory if memory use is acceptable.
- [ ] If necessary, add an LRU texture cache retaining visible and nearby
  thumbnails.
- [ ] Keep failed-thumbnail state cached for the current directory to prevent
  repeated extraction attempts.

### 11. Add Open Folder Later

- [ ] After the open-image workflow is stable, add `File > Open Folder`.
- [ ] Scan the selected folder using the same model API.
- [ ] Select and load the first supported image, unless product behavior is
  changed explicitly.
- [ ] Handle a folder containing no supported files with a clear empty state.
- [ ] Preserve the existing `File > Open` behavior.

## Automated Tests

- [ ] Test case-insensitive supported-extension matching.
- [ ] Test folder scanning excludes directories and unsupported files.
- [ ] Test deterministic or natural filename ordering.
- [ ] Test locating the initially opened file after sorting.
- [ ] Test previous and next selection at list boundaries.
- [ ] Test that directory changes increment the directory generation.
- [ ] Test stale thumbnail results are rejected.
- [ ] Test duplicate thumbnail jobs are not queued.
- [ ] Test failed thumbnail state prevents repeated work.
- [ ] Test aspect-ratio-preserving thumbnail dimensions.
- [ ] Test RGB-to-RGBA normalization.
- [ ] Test malformed embedded JPEG data fails safely.
- [ ] Test missing embedded previews do not invoke full RAW decoding.

Keep OpenGL texture operations out of ordinary unit tests unless a controlled
OpenGL context is explicitly created.

## Manual Verification

- [ ] Open a RAW and confirm its parent directory populates the filmstrip.
- [ ] Confirm the opened file is selected and scrolled into view.
- [ ] Click multiple images rapidly and confirm stale images never replace the
  newest selection.
- [ ] Verify the selected image still shows its embedded preview before the full
  decode.
- [ ] Navigate with left and right arrow keys.
- [ ] Verify arrow keys do not interfere with active edit controls.
- [ ] Scroll through a directory containing hundreds or thousands of RAW files.
- [ ] Confirm only visible and nearby thumbnails are decoded.
- [ ] Test files with JPEG previews, bitmap previews, missing previews, and
  corrupt previews.
- [ ] Test portrait and landscape images.
- [ ] Change folders while thumbnail work is active.
- [ ] Close the application while viewer and thumbnail jobs are active.
- [ ] Watch CPU, memory, and GPU texture usage while browsing.
- [ ] Confirm all OpenGL creation and deletion occurs on the main thread.
- [ ] Run the normal build and tests with the platform's CMake preset (`linux`
  on Debian or `windows` from a Visual Studio Developer Command Prompt).

## Suggested Milestones

### Milestone 1: Navigation Skeleton

- Folder scan and sorting.
- Filmstrip model.
- Placeholder-only horizontal UI.
- Mouse and keyboard selection.
- Existing viewer loader integration.

### Milestone 2: Thumbnail Pipeline

- Fixed worker queue.
- Embedded RAW preview extraction.
- CPU resizing and RGBA8 normalization.
- Main-thread OpenGL uploads and cleanup.

### Milestone 3: Scalability and Polish

- Visible-range scheduling and prefetch.
- Memory and queue bounds.
- Automatic scroll-to-selection.
- Improved placeholders, tooltips, and orientation handling.
- Optional `Open Folder` workflow.

## Completion Criteria

- Opening an image populates a persistent filmstrip from its parent directory.
- Clicking or keyboard-selecting an item loads it through the existing
  preview-first pipeline.
- Rapid navigation cannot display stale viewer or thumbnail results.
- Filmstrip thumbnails use embedded previews and never full RAW processing.
- Large directories remain responsive through bounded, lazy thumbnail work.
- Missing or invalid previews display a stable placeholder without repeated
  work.
- OpenGL texture creation and deletion occur only on the main thread.
- Changing directories and shutting down release thumbnail CPU buffers, GPU
  textures, futures, and worker threads safely.
