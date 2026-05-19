#pragma once
#include <future>
#include <optional>
#include <GLFW/glfw3.h>
#include "ImageLoader.h"

/*
std::future provides a mechanism to access the result of asynchronous operations
std::ptional - to return a valor when its needed but if not, there's no problem

*/

class App {
public:
    bool init();
    void run();
    void shutdown();

private:

    void renderUI();
    void renderMenuBar();
    void renderDevelopPanel();
    void renderToolPanel();
    void renderDockSpace();
    void photoViewer();

    GLFWwindow* m_window = nullptr;
    std::optional<RawImage> m_image;
    // m_loadFuture carries the result of the background decodeRawImage() call
    std::future<std::optional<RawImageData>> m_loadFuture;
    // shows "Loading..." in photoViewer()
    bool m_loading = false;

    // Adjustments
    float m_exposure = 0.0f;
    float m_color[3] = { 1.0f, 1.0f, 1.0f };
    float m_zoom = 1.0f;
};
