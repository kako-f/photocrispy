cmake_minimum_required(VERSION 3.16.0)
project(photocrispy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(libraw CONFIG REQUIRED)

# Define Source Files
# Core logic and the specific backends for GLFW and OpenGL
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
set(IMGUIDIALOG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuiFileDialog")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Executables
add_executable(photocrispy
    src/main.cpp
    src/App.cpp
    src/ImageLoader.cpp
    src/StbImageDecoder.cpp
    src/FileBrowser.cpp
    ${IMGUI_SOURCES}
    ${IMGUIDIALOG_DIR}/ImGuiFileDialog.cpp
)

# Include Headers
# Where to look for #include "imgui.h" and others
target_include_directories(photocrispy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUIDIALOG_DIR}
)
target_link_libraries(photocrispy PRIVATE
    libraw::raw
    fmt::fmt
    glfw
    OpenGL::GL
)

# Testing
enable_testing()

add_executable(load_result_queue_tests
    tests/LoadResultQueueTests.cpp
)

target_include_directories(load_result_queue_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME load_result_queue_tests COMMAND load_result_queue_tests)

add_executable(image_loader_data_tests
    tests/ImageLoaderDataTests.cpp
    src/StbImageDecoder.cpp
)

target_include_directories(image_loader_data_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${IMGUIDIALOG_DIR}
)

add_test(NAME image_loader_data_tests COMMAND image_loader_data_tests)
