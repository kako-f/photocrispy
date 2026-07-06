#pragma once
#include <future>
#include <optional>
#include <string>
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
    std::string m_lastDir = "."; 
    void renderUI();
    void renderMenuBar();
    void renderDevelopPanel();
    void renderDockSpace();
    void photoViewer();
    void registerSettingsHandler();
    void clearImage();

    GLFWwindow* m_window = nullptr;
    std::optional<RawImage> m_image;
    // m_loadFuture carries the result of the background decodeRawImage() call
    std::future<std::optional<ImageData>> m_loadFuture;
    // shows "Loading..." in photoViewer()
    bool m_loading = false;

    // Adjustments
    float m_exposure = 0.0f;
    float m_color[3] = { 1.0f, 1.0f, 1.0f };
    float m_zoom = 1.0f;
};
