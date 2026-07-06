#pragma once
#include <cstdint>
#include <future>
#include <optional>
#include <string>
#include <GLFW/glfw3.h>
#include "ImageLoader.h"
#include "LoadResultQueue.h"

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
    // Background loading emits preview and full images into this queue.
    std::future<void> m_loadFuture;
    LoadResultQueue<LoadResult> m_loadResults;
    uint64_t m_loadGeneration = 0;
    // shows "Loading..." in photoViewer()
    bool m_loading = false;
    bool m_showingPreview = false;

    // Adjustments
    float m_exposure = 0.0f;
    float m_color[3] = { 1.0f, 1.0f, 1.0f };
    float m_zoom = 1.0f;
};
