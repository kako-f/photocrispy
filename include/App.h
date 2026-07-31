#pragma once
#include "FileBrowser.h"
#include "ImageLoader.h"
#include "LoadResultQueue.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <future>
#include <optional>
#include <string>
#include <vector>

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
  void filmStrip();
  void drawAboutWindow();

  void registerSettingsHandler();
  void clearImage();

  void openNewFile(const fs::path &path);

  GLFWwindow *m_window = nullptr;
  std::optional<RawImage> m_image;
  // FileBrowser
  FileBrowser browser;
  // Background loading emits preview and full images into this queue.
  std::vector<std::future<void>> m_loadFutures;
  LoadResultQueue<LoadResult> m_loadResults;
  LoadResultQueue<uint64_t> m_completedLoads;
  uint64_t m_loadGeneration = 0;
  // shows "Loading..." in photoViewer()
  bool m_loading = false;
  bool m_showingPreview = false;
  bool m_showAboutWindow = false;

  // Adjustments
  float m_exposure = 0.0f;
  float m_color[3] = {1.0f, 1.0f, 1.0f};
  float m_zoom = 1.0f;
};
