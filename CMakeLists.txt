cmake_minimum_required(VERSION 3.10.0)
project(photocrispy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(libraw CONFIG REQUIRED)

# 2. Define ImGui Source Files
# Core logic and the specific backends for GLFW and OpenGL
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
set(IMGUIDIALOG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/ImguiFileDialog")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)


add_executable(photocrispy main.cpp App.cpp ImageLoader.cpp ${IMGUI_SOURCES} ${IMGUIDIALOG_DIR}/ImGuiFileDialog.cpp)

# 4. Include Headers
# Where to look for #include "imgui.h"
target_include_directories(photocrispy PRIVATE
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