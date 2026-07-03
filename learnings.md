* we are building outside the main directory.
* "CMakePreset.json" dictates where to build the program
* Ninja is bundle with VS through the installer.
    * Cmake extension needs a kit selected so it knows which compiler and tools to use
    * don't specify a kit in the "CMakePreset.json" so CMake extension use whichever generator is available.
