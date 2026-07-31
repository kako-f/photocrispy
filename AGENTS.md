# AGENTS.md

Local working notes for coding agents.

## Current Status

- PhotoCrispy is a C++17 desktop RAW image viewer/editor using GLFW, OpenGL,
  ImGui, LibRaw, stb_image, and fmt.
- Production implementations live in `src/`; project headers live in `include/`.
  Tests remain in `tests/`, shaders in `shaders/`, and vendored code in `external/`.
- The app has file browsing, a docked viewer and filmstrip, zoom/pan controls,
  and preview-first asynchronous RAW loading followed by a full 16-bit decode.
- `src/about.cpp` and `include/about.h` are placeholders and are not part of the
  current CMake target.
- The working tree contains in-progress, uncommitted application changes; do
  not discard or overwrite them while working on another task.

## Build

Use the Visual Studio Developer Command Prompt so MSVC, CMake, and the compiler
toolchain are on PATH.

```powershell
cmake --preset windows
cmake --build --preset windows
ctest --preset windows
```

Regular PowerShell may not have `cmake` or MSVC on `PATH`; use the Developer
Command Prompt for build and test verification.

On Linux, use the matching presets:

```bash
cmake --preset linux
cmake --build --preset linux
ctest --preset linux
```

## Notes

- Dependencies are managed through vcpkg manifest mode.
- Build output is configured outside the synced repository at `D:\Build\PhotoCrispy`.
- Do not modify vendored dependencies under `external/` unless explicitly requested.
- RAW loading is preview-first: worker jobs may emit an embedded preview, then
  the full 16-bit decode. OpenGL texture creation and deletion must stay on the
  main thread.
- Keep outstanding RAW loader futures alive until they complete. Do not assign
  over a still-running `std::future` from `std::async(std::launch::async, ...)`;
  doing so can block the UI when opening a second file during decode.
- Use generation IDs to discard stale loader results when a newer file has been
  opened.
