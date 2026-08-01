# Filmstrip Refresh Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Refresh the filmstrip only when opening a file from a different directory.

**Architecture:** `App` records the directory whose files are cached by `FileBrowser`. File opening refreshes that cache when the selected file's parent changes; frame rendering only draws it.

**Tech Stack:** C++17, Dear ImGui, CMake/CTest

## Global Constraints

- Do not add filesystem watching or a manual refresh action.
- Preserve the current throwing filesystem behavior for a later, separate fix.
- Do not add a test-only seam to the ImGui application loop.

---

### Task 1: Cache the filmstrip directory

**Files:**
- Modify: `include/App.h`
- Modify: `src/App.cpp:119-139,334-348`

**Interfaces:**
- Consumes: `FileBrowser::Refresh(const fs::path &folder)`
- Produces: `App::m_filmstripDir`, the directory represented by the browser's cached file list

- [ ] **Step 1: Confirm the current behavior**

Inspect `App::filmStrip()` and confirm that `browser.Refresh(m_lastDir)` is inside the per-frame rendering path.

- [ ] **Step 2: Add the cached directory state**

Add this member beside `m_lastDir` in `include/App.h`:

```cpp
fs::path m_filmstripDir;
```

- [ ] **Step 3: Refresh when opening a different directory**

At the start of `App::openNewFile()`, derive the parent once and refresh conditionally:

```cpp
const fs::path directory = path.parent_path();
if (directory != m_filmstripDir) {
  browser.Refresh(directory);
  m_filmstripDir = directory;
}

m_lastDir = directory.string();
```

Keep the existing generation and asynchronous loading logic unchanged.

- [ ] **Step 4: Remove the per-frame scan**

Change `App::filmStrip()` so the `m_lastDir != "."` block only calls `browser.Draw()` and opens the selected file. Remove `browser.Refresh(m_lastDir)`.

- [ ] **Step 5: Build and run all tests**

Run:

```bash
cmake --preset linux
cmake --build --preset linux
ctest --preset linux
```

Expected: configuration and build succeed; both existing tests pass.

- [ ] **Step 6: Review the diff**

Run:

```bash
git diff --check
git diff -- include/App.h src/App.cpp
```

Expected: only the directory cache, conditional refresh, and removal of the per-frame refresh are present.
