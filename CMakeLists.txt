cmake_minimum_required(VERSION 3.10.0)
project(photoraw VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

## Find dependencies
find_package(glfw3 REQUIRED)
find_package(libraw CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

include_directories("${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/include")
include_directories("include")

# Set ImGui path to the submodule
set(IMGUI_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/share/opencv4")
find_package(OpenCV REQUIRED)

# Gather ImGui source files
file(GLOB IMGUI_SRC
    ${IMGUI_PATH}/*.cpp
    ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp
    ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
)

add_library(ImGui STATIC ${IMGUI_SRC})

target_include_directories(ImGui PUBLIC
    ${IMGUI_PATH}
    ${IMGUI_PATH}/backends
)

add_executable(photoraw 
        src/photocrispy.cpp 
        main.cpp 
        src/raw_processing.cpp 
        src/gl_photo_texture.cpp 
        src/imageviewer.cpp
        src/threaded_load.cpp
        src/content_browser.cpp
        src/imageutils.cpp
        src/image_processing.cpp
        src/opengl_rendering.cpp
        src/photo_shader.cpp
        )

target_link_libraries(photoraw
    PRIVATE
        glfw
        opengl32.lib
        ImGui
        fmt::fmt
        libraw::raw_r
        nfd::nfd
        glad::glad
)