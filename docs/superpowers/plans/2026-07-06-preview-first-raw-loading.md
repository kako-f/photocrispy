# Preview-First RAW Loading Implementation Plan

**Status:** Tasks 1-5 are implemented in the current tree. The Debian build and
automated tests pass. Task 6 manual RAW verification and Windows verification
remain.
The unchecked boxes below preserve the original execution workflow; this status
line is the current implementation record.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show an embedded RAW preview quickly, then replace it with the full 16-bit RAW decode when ready.

**Architecture:** Add format-aware image data, thread-safe result/completion queues, and background load jobs that emit preview and full results tagged with generation IDs. `App` keeps outstanding futures until they complete, polls results on the main thread, and remains the only place that creates or deletes OpenGL textures.

**Tech Stack:** C++17, LibRaw, OpenGL/GLFW, ImGui, stb_image through a project-owned wrapper, CMake/vcpkg, GCC on Debian, and MSVC on Windows.

---

## File Structure

- `ImageLoader.h`: Owns image payload types, loader function declarations, and texture upload declaration.
- `ImageLoader.cpp`: Owns LibRaw preview extraction, full RAW decode, and format-aware OpenGL texture upload.
- `StbImageDecoder.h`: Declares a tiny JPEG memory decoder wrapper.
- `StbImageDecoder.cpp`: Defines `STB_IMAGE_IMPLEMENTATION` once and calls the vendored `stb_image.h` without modifying `external/`.
- `LoadResultQueue.h`: Header-only thread-safe FIFO queue for background loader results.
- `App.h`: Replaces the single final-result future with load future storage, result/completion queues, generation, and background job state.
- `App.cpp`: Starts preview-first load jobs, polls queue results, discards stale generations, and updates the viewer status.
- `tests/LoadResultQueueTests.cpp`: Assert-based unit tests for FIFO and empty queue behavior.
- `tests/ImageLoaderDataTests.cpp`: Assert-based unit tests for image data format helpers.
- `CMakeLists.txt`: Adds new source files and lightweight assert-based test executables.

## Current Implementation Notes

The implemented app uses `std::vector<std::future<void>> m_loadFutures`, not a
single `std::future`. This was added after manual verification showed that
opening file B while file A was still decoding could briefly hang the UI. The
root cause was assigning over a still-running `std::future` from
`std::async(std::launch::async, ...)`, which can block until the previous job
finishes.

The app also uses two queues:

- `LoadResultQueue<LoadResult> m_loadResults` for preview/full image payloads.
- `LoadResultQueue<uint64_t> m_completedLoads` for generation completion.

`photoViewer()` polls both queues and prunes completed futures each frame.
Texture creation and deletion remain on the main thread.

Tests are split into `load_result_queue_tests` and `image_loader_data_tests`
instead of one combined executable, because the simple assert-based test files
each define their own `main()`.

Build verification uses the matching platform preset:

```sh
cmake --preset linux
cmake --build --preset linux
ctest --preset linux
```

On Windows, use a Visual Studio Developer Command Prompt:

```powershell
cmake --preset windows
cmake --build --preset windows
ctest --preset windows
```

---

### Task 1: Add A Lightweight Test Target And Result Queue

**Files:**
- Create: `LoadResultQueue.h`
- Create: `tests/LoadResultQueueTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `tests/LoadResultQueueTests.cpp`:

```cpp
#include "LoadResultQueue.h"

#include <cassert>
#include <optional>

struct QueueItem {
    int value = 0;
};

static void emptyQueueReturnsNoValue()
{
    LoadResultQueue<QueueItem> queue;
    assert(!queue.tryPop().has_value());
}

static void queueReturnsItemsInFifoOrder()
{
    LoadResultQueue<QueueItem> queue;
    queue.push(QueueItem{1});
    queue.push(QueueItem{2});

    auto first = queue.tryPop();
    auto second = queue.tryPop();
    auto third = queue.tryPop();

    assert(first.has_value());
    assert(second.has_value());
    assert(first->value == 1);
    assert(second->value == 2);
    assert(!third.has_value());
}

int main()
{
    emptyQueueReturnsNoValue();
    queueReturnsItemsInFifoOrder();
    return 0;
}
```

Modify `CMakeLists.txt` so the test is part of the build before the queue exists:

```cmake
enable_testing()

add_executable(load_result_queue_tests
    tests/LoadResultQueueTests.cpp
)

target_include_directories(load_result_queue_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME load_result_queue_tests COMMAND load_result_queue_tests)
```

- [ ] **Step 2: Run the test to verify it fails**

Run from Visual Studio Developer Command Prompt:

```powershell
cmake --preset vcpkg
cmake --build D:\Build\PhotoCrispy
ctest --test-dir D:\Build\PhotoCrispy -C Debug --output-on-failure
```

Expected: build fails because `LoadResultQueue.h` does not exist.

- [ ] **Step 3: Add the minimal queue implementation**

Create `LoadResultQueue.h`:

```cpp
#pragma once

#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class LoadResultQueue {
public:
    void push(T value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(value));
    }

    std::optional<T> tryPop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return std::nullopt;

        T value = std::move(m_queue.front());
        m_queue.pop();
        return value;
    }

private:
    std::mutex m_mutex;
    std::queue<T> m_queue;
};
```

- [ ] **Step 4: Run the test to verify it passes**

Run:

```powershell
cmake --preset vcpkg
cmake --build D:\Build\PhotoCrispy
ctest --test-dir D:\Build\PhotoCrispy -C Debug --output-on-failure
```

Expected: `load_result_queue_tests` passes.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt LoadResultQueue.h tests/LoadResultQueueTests.cpp
git commit -m "test: add load result queue coverage"
```

---

### Task 2: Add Format-Aware Image Data

**Files:**
- Modify: `ImageLoader.h`
- Create: `tests/ImageLoaderDataTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing data model test**

Create `tests/ImageLoaderDataTests.cpp`:

```cpp
#include "ImageLoader.h"

#include <cassert>
#include <cstdint>

static void previewDataUses8BitPixels()
{
    ImageData image;
    image.kind = ImageKind::Preview;
    image.is16Bit = false;
    image.channels = 3;
    image.pixels8 = { 1, 2, 3 };

    assert(image.kind == ImageKind::Preview);
    assert(!image.is16Bit);
    assert(image.channels == 3);
    assert(image.pixels8.size() == 3);
    assert(image.pixels16.empty());
}

static void fullDataUses16BitPixels()
{
    ImageData image;
    image.kind = ImageKind::Full;
    image.is16Bit = true;
    image.channels = 3;
    image.pixels16 = { 1024, 2048, 4096 };

    assert(image.kind == ImageKind::Full);
    assert(image.is16Bit);
    assert(image.channels == 3);
    assert(image.pixels16.size() == 3);
    assert(image.pixels8.empty());
}

int main()
{
    previewDataUses8BitPixels();
    fullDataUses16BitPixels();
    return 0;
}
```

Add the file to a separate `image_loader_data_tests` executable in
`CMakeLists.txt`.

- [ ] **Step 2: Run the test to verify it fails**

Run:

```powershell
cmake --build D:\Build\PhotoCrispy
ctest --test-dir D:\Build\PhotoCrispy -C Debug --output-on-failure
```

Expected: build fails because `ImageKind` and `ImageData` do not exist.

- [ ] **Step 3: Add the minimal data model**

Modify `ImageLoader.h` so the image types include:

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

struct RawImage
{
    unsigned int textureId = 0;
    int width = 0;
    int height = 0;
    ImageKind kind = ImageKind::Full;
};
```

Keep a temporary compatibility alias while migrating:

```cpp
using RawImageData = ImageData;
```

- [ ] **Step 4: Run the test to verify it passes**

Run:

```powershell
cmake --build D:\Build\PhotoCrispy
ctest --test-dir D:\Build\PhotoCrispy -C Debug --output-on-failure
```

Expected: `load_result_queue_tests` and `image_loader_data_tests` pass.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt ImageLoader.h tests/ImageLoaderDataTests.cpp
git commit -m "feat: add format-aware image data"
```

---

### Task 3: Add A Project-Owned stb_image JPEG Decoder Wrapper

**Files:**
- Create: `StbImageDecoder.h`
- Create: `StbImageDecoder.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/ImageLoaderDataTests.cpp`

- [ ] **Step 1: Write the failing decoder test**

Append this test to `tests/ImageLoaderDataTests.cpp`:

```cpp
#include "StbImageDecoder.h"

static void invalidJpegMemoryReturnsNoImage()
{
    const uint8_t invalid[] = { 0, 1, 2, 3 };
    auto image = decodeJpegMemoryToRgb(invalid, sizeof(invalid), ImageKind::Preview);
    assert(!image.has_value());
}
```

Call it from `main()`:

```cpp
invalidJpegMemoryReturnsNoImage();
```

- [ ] **Step 2: Run the test to verify it fails**

Run:

```powershell
cmake --build D:\Build\PhotoCrispy
ctest --test-dir D:\Build\PhotoCrispy -C Debug --output-on-failure
```

Expected: build fails because `StbImageDecoder.h` does not exist.

- [ ] **Step 3: Add the minimal decoder wrapper**

Create `StbImageDecoder.h`:

```cpp
#pragma once

#include "ImageLoader.h"

#include <cstddef>
#include <cstdint>
#include <optional>

std::optional<ImageData> decodeJpegMemoryToRgb(const uint8_t* data, size_t size, ImageKind kind);
```

Create `StbImageDecoder.cpp`:

```cpp
#include "StbImageDecoder.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

std::optional<ImageData> decodeJpegMemoryToRgb(const uint8_t* data, size_t size, ImageKind kind)
{
    if (!data || size == 0)
        return std::nullopt;

    int width = 0;
    int height = 0;
    int sourceChannels = 0;
    constexpr int requestedChannels = 3;

    unsigned char* decoded = stbi_load_from_memory(
        data,
        static_cast<int>(size),
        &width,
        &height,
        &sourceChannels,
        requestedChannels);

    if (!decoded)
        return std::nullopt;

    ImageData image;
    image.width = width;
    image.height = height;
    image.channels = requestedChannels;
    image.is16Bit = false;
    image.kind = kind;
    image.pixels8.assign(decoded, decoded + (width * height * requestedChannels));

    stbi_image_free(decoded);
    return image;
}
```

Add `StbImageDecoder.cpp` to both `photocrispy` and `image_loader_data_tests`,
and add `${IMGUIDIALOG_DIR}` to the test include directories so
`"stb/stb_image.h"` resolves.

- [ ] **Step 4: Run the test to verify it passes**

Run:

```powershell
cmake --preset vcpkg
cmake --build D:\Build\PhotoCrispy
ctest --test-dir D:\Build\PhotoCrispy -C Debug --output-on-failure
```

Expected: tests pass.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt StbImageDecoder.h StbImageDecoder.cpp tests/ImageLoaderDataTests.cpp
git commit -m "feat: add jpeg memory decoder"
```

---

### Task 4: Split Full Decode And Add Embedded Preview Extraction

**Files:**
- Modify: `ImageLoader.h`
- Modify: `ImageLoader.cpp`

- [ ] **Step 1: Rename the full decode API in the header**

Modify declarations in `ImageLoader.h`:

```cpp
std::optional<ImageData> decodeFullRawImage(const std::string& path);
std::optional<ImageData> extractEmbeddedPreviewImage(const std::string& path);
RawImage uploadTexture(const ImageData& data);
```

Remove the old `decodeRawImage()` declaration once all references are updated in this task.

- [ ] **Step 2: Update full decode implementation**

Rename `decodeRawImage()` to `decodeFullRawImage()` and set the new fields:

```cpp
ImageData result;
result.width = img->width;
result.height = img->height;
result.channels = 3;
result.is16Bit = true;
result.kind = ImageKind::Full;
result.pixels16.assign(src, src + pixelCount);
```

- [ ] **Step 3: Add embedded preview extraction**

Add to `ImageLoader.cpp`:

```cpp
std::optional<ImageData> extractEmbeddedPreviewImage(const std::string& path)
{
    LibRaw raw;

    if (raw.open_file(path.c_str()) != LIBRAW_SUCCESS)
        return std::nullopt;

    if (raw.unpack_thumb() != LIBRAW_SUCCESS)
        return std::nullopt;

    libraw_processed_image_t* thumb = raw.dcraw_make_mem_thumb();
    if (!thumb)
        return std::nullopt;

    std::optional<ImageData> result;

    if (thumb->type == LIBRAW_IMAGE_JPEG) {
        result = decodeJpegMemoryToRgb(thumb->data, thumb->data_size, ImageKind::Preview);
    } else if (thumb->type == LIBRAW_IMAGE_BITMAP && thumb->colors >= 3) {
        ImageData image;
        image.width = thumb->width;
        image.height = thumb->height;
        image.channels = 3;
        image.is16Bit = false;
        image.kind = ImageKind::Preview;

        const int sourceChannels = thumb->colors;
        const int pixelCount = thumb->width * thumb->height;
        image.pixels8.reserve(pixelCount * 3);

        for (int i = 0; i < pixelCount; ++i) {
            const int source = i * sourceChannels;
            image.pixels8.push_back(thumb->data[source + 0]);
            image.pixels8.push_back(thumb->data[source + 1]);
            image.pixels8.push_back(thumb->data[source + 2]);
        }

        result = std::move(image);
    }

    LibRaw::dcraw_clear_mem(thumb);
    return result;
}
```

Add `#include "StbImageDecoder.h"` at the top of `ImageLoader.cpp`.

- [ ] **Step 4: Make texture upload format-aware**

Update `uploadTexture()`:

```cpp
RawImage uploadTexture(const ImageData& data)
{
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (data.is16Bit) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, data.width, data.height,
                     0, GL_RGB, GL_UNSIGNED_SHORT, data.pixels16.data());
    } else {
        GLenum format = data.channels == 4 ? GL_RGBA : GL_RGB;
        GLint internalFormat = data.channels == 4 ? GL_RGBA8 : GL_RGB8;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, data.width, data.height,
                     0, format, GL_UNSIGNED_BYTE, data.pixels8.data());
    }

    return RawImage{ (unsigned int)texId, data.width, data.height, data.kind };
}
```

- [ ] **Step 5: Build**

Run:

```powershell
cmake --build D:\Build\PhotoCrispy
ctest --test-dir D:\Build\PhotoCrispy -C Debug --output-on-failure
```

Expected: app and tests build.

- [ ] **Step 6: Commit**

```powershell
git add ImageLoader.h ImageLoader.cpp
git commit -m "feat: extract embedded raw previews"
```

---

### Task 5: Integrate Preview-First Loading Into App State

**Files:**
- Modify: `App.h`
- Modify: `App.cpp`

- [ ] **Step 1: Replace single-result future state**

Modify `App.h` includes:

```cpp
#include <cstdint>
#include "LoadResultQueue.h"
```

Replace:

```cpp
std::future<std::optional<ImageData>> m_loadFuture;
bool m_loading = false;
```

With:

```cpp
std::vector<std::future<void>> m_loadFutures;
LoadResultQueue<LoadResult> m_loadResults;
LoadResultQueue<uint64_t> m_completedLoads;
uint64_t m_loadGeneration = 0;
bool m_loading = false;
bool m_showingPreview = false;
```

- [ ] **Step 2: Start a background job that emits preview then full**

In `renderMenuBar()`, replace the current async decode call:

```cpp
const uint64_t generation = ++m_loadGeneration;
m_loading = true;
m_showingPreview = false;

m_loadFutures.push_back(std::async(std::launch::async, [this, filePathName, generation]() {
    if (auto preview = extractEmbeddedPreviewImage(filePathName)) {
        m_loadResults.push(LoadResult{ generation, std::move(*preview) });
    }

    if (auto full = decodeFullRawImage(filePathName)) {
        m_loadResults.push(LoadResult{ generation, std::move(*full) });
    }

    m_completedLoads.push(generation);
}));
```

Keep every returned future in `m_loadFutures`. Do not overwrite a single
`std::future`; replacing a still-running async future can block the UI thread
while the old LibRaw decode finishes.

- [ ] **Step 3: Poll queued results in `photoViewer()`**

Replace the future polling block with:

```cpp
while (auto result = m_loadResults.tryPop()) {
    if (result->generation != m_loadGeneration)
        continue;

    clearImage();
    m_image = uploadTexture(result->image);
    m_showingPreview = result->image.kind == ImageKind::Preview;

    if (result->image.kind == ImageKind::Full) {
        m_loading = false;
        m_showingPreview = false;
    }
}

for (auto it = m_loadFutures.begin(); it != m_loadFutures.end();) {
    if (it->valid() && it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        it->get();
        it = m_loadFutures.erase(it);
    } else {
        ++it;
    }
}

while (auto completedGeneration = m_completedLoads.tryPop()) {
    if (*completedGeneration == m_loadGeneration)
        m_loading = false;
}
```

- [ ] **Step 4: Keep preview visible while full is loading**

Change the viewer branch order so image display takes precedence over the spinner:

```cpp
if (m_image.has_value())
{
    // existing image drawing code
    if (m_loading && m_showingPreview)
        ImGui::TextDisabled("Loading full RAW...");
}
else if (m_loading)
{
    // existing spinner code
}
else
{
    ImGui::TextDisabled("No image loaded. Use File > Open.");
}
```

This keeps the old image visible until the preview or full image arrives, and keeps the preview visible while full decode continues.

- [ ] **Step 5: Wait for outstanding loaders during shutdown**

In `shutdown()`, wait for all valid futures before destroying app state:

```cpp
for (auto& future : m_loadFutures) {
    if (future.valid())
        future.wait();
}
```

- [ ] **Step 6: Build**

Run:

```powershell
cmake --build D:\Build\PhotoCrispy
ctest --test-dir D:\Build\PhotoCrispy -C Debug --output-on-failure
```

Expected: app and tests build.

- [ ] **Step 7: Commit**

```powershell
git add App.h App.cpp
git commit -m "feat: load raw previews before full decode"
```

---

### Task 6: Manual Verification With RAW Files

**Files:**
- No code changes expected unless verification finds a bug.

- [ ] **Step 1: Launch from Visual Studio Developer Command Prompt**

Run:

```powershell
D:\Build\PhotoCrispy\photocrispy.exe
```

- [ ] **Step 2: Verify preview-first behavior**

Open a RAW file known to contain an embedded JPEG preview.

Expected:
- The viewer shows a preview before full RAW processing completes.
- The UI still indicates that the full RAW is loading while the preview is shown.
- The preview is replaced by the full image.

- [ ] **Step 3: Verify fallback behavior**

Open a RAW file with no embedded preview, or temporarily force `extractEmbeddedPreviewImage()` to return `std::nullopt`.

Expected:
- The loading indicator remains visible.
- Full RAW decode still completes.
- Missing preview does not fail the open operation.

- [ ] **Step 4: Verify stale result behavior**

Open file A, then quickly open file B before A finishes.

Expected:
- Results from file A do not replace file B after B starts loading.
- Opening file B does not make the UI hang while file A continues decoding in
  the background.

- [ ] **Step 5: Commit any verification fixes**

Only if verification required code changes:

Use a specific `git add` command for whichever files were changed during
verification. For example, if the fix is in the app integration:

```powershell
git add App.h App.cpp
git commit -m "fix: stabilize preview-first raw loading"
```
