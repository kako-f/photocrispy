# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

Dependencies are managed by vcpkg (manifest mode). Build output goes to `D:\Build\PhotoCrispy` (outside the synced Drive folder).

```powershell
# Configure (first time or after CMakeLists changes)
cmake --preset default

# Build
cmake --build D:\Build\PhotoCrispy

# Run
D:\Build\PhotoCrispy\Debug\photocrispy.exe
```

## Architecture

**Entry point:** `main.cpp` constructs an `App`, calls `init()` → `run()` → `shutdown()`.

**`App`** owns the entire application lifecycle:
- `run()` is the main loop: poll GLFW events → ImGui new frame → `renderUI()` → render → swap buffers
- `renderUI()` calls each panel in order: `renderDockSpace()`, `renderMenuBar()`, `photoViewer()`, `renderDevelopPanel()`, `renderToolPanel()`
- UI state (`m_exposure`, `m_color`, `m_image`, `m_loading`, `m_loadFuture`) lives as members of `App`

**`ImageLoader`** handles RAW file ingestion in two deliberately separate phases:

| Function | Thread | What it does |
|---|---|---|
| `decodeRawImage(path)` | Any thread | LibRaw decode → CPU pixel buffer (`RawImageData`) |
| `uploadTexture(data)` | Main thread only | OpenGL texture upload → `RawImage` (GPU handle) |

**Critical constraint:** OpenGL calls must happen on the main thread (the thread that created the GLFW context). `decodeRawImage` is safe to run off-thread; `uploadTexture` is not.

**Async loading flow:**
1. File picked → `std::async(decodeRawImage, path)` stored in `m_loadFuture`, `m_loading = true`
2. Each frame in `photoViewer()` → `wait_for(0)` polls without blocking
3. When ready → `uploadTexture()` called on the main thread → `m_image` set, `m_loading = false`

**ImGui/vendored libraries** are compiled directly from source (not linked as libraries):
- `external/imgui/` — core ImGui + GLFW and OpenGL3 backends
- `external/ImguiFileDialog/` — file picker dialog

The dockspace (`renderDockSpace`) uses `ImGuiDockNodeFlags_PassthruCentralNode` so the GLFW clear color shows through the central area when no panel is docked there.

## Key data types

```cpp
RawImageData   // CPU-side: std::vector<uint16_t> pixels + width/height
RawImage       // GPU-side: unsigned int textureId + width/height
```

Textures are uploaded as `GL_RGB16` (16-bit per channel). Display currently has no gamma correction — a `pow(color, 1/2.2)` pass or tone mapper is needed for perceptually correct output.
