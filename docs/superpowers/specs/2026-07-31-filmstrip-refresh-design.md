# Filmstrip Refresh Design

## Goal

Stop scanning the current directory every rendered frame. Refresh the filmstrip only when opening a file whose parent directory differs from the directory currently displayed.

## Design

`App` stores the filmstrip's current directory. `openNewFile()` compares the selected file's parent against that value and calls `FileBrowser::Refresh()` only when they differ. `filmStrip()` only draws the cached file list.

No filesystem watcher or manual refresh is included. Files added to the current directory while the app is running appear after opening a file from another directory and returning.

## Error handling and verification

This change preserves existing filesystem error behavior; handling inaccessible directories is a separate fix. Verification consists of the existing configure, build, and test presets because the scheduling behavior is coupled to the ImGui application loop and adding a test seam would exceed the change itself.
