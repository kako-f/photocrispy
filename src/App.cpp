#include "App.h"
#include "ImGuiFileDialog.h"
#include "ImageLoader.h"
#include "fmt/core.h"
#include "graphics/shaderProgram.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fmt/base.h>
#include <future>
#include <utility>

bool App::init() {
  // Setup

  if (!glfwInit())
    return false;

  m_window = glfwCreateWindow(1280, 720, "PhotoCrispy", NULL, NULL);

  if (!m_window)
    return false;

  glfwMakeContextCurrent(m_window);
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    return false;

  glfwSwapInterval(1); // Enable vsync

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  registerSettingsHandler();

  ImGuiIO &io = ImGui::GetIO();

  // Docking
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui_ImplGlfw_InitForOpenGL(m_window, true);
  ImGui_ImplOpenGL3_Init("#version 430");

  // Triangle
  newTriangle.createTriangle();
  newTriangle.createFramebuffer();

  // image processing start
  m_processingReady = initImageProcessing();

  return true;
}

bool App::initImageProcessing() {

  m_imageProcessingShader.create_shader(
      PHOTOCRISPY_SHADER_DIR "/imageProcessing.vert",
      PHOTOCRISPY_SHADER_DIR "/imageProcessing.frag");

  GLint linked = false;
  glGetProgramiv(m_imageProcessingShader.ID, GL_LINK_STATUS, &linked);
  // checking status of the program
  if (linked != GL_TRUE) {
    glDeleteProgram(m_imageProcessingShader.ID);
    m_imageProcessingShader.ID = 0;
    return false;
  }
  glGenVertexArrays(1, &m_processingVao);
  glGenFramebuffers(1, &m_processingFramebuffer);
  glGenTextures(1, &m_processedTexture);

  glBindTexture(GL_TEXTURE_2D, m_processedTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindFramebuffer(GL_FRAMEBUFFER, m_processingFramebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_processedTexture, 0);
  // use the shader
  m_imageProcessingShader.use();
  glUniform1i(glGetUniformLocation(m_imageProcessingShader.ID, "sourceImage"),
              0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUseProgram(0);

  return true;
}

void App::resizeProcessedImage(int width, int height) {
  // onlye when image dimensions changes
  // TODO = NECESARRY?
  if (!m_processingReady || width <= 0 || height <= 0)
    return;

  if (width == m_processedWidth && height == m_processedHeight)
    return;

  glBindTexture(GL_TEXTURE_2D, m_processedTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, m_processingFramebuffer);
  const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  if (status != GL_FRAMEBUFFER_COMPLETE) {
    fmt::print(stderr, "Image processing framebuffer incomplete: {:#x}\n",
               status);
    m_processingReady = false;
    return;
  }

  m_processedWidth = width;
  m_processedHeight = height;
}

void App::processImage() {
  if (!m_processingReady || !m_processingDirty || !m_image.has_value())
    return;
  if (m_processedWidth != m_image->width ||
      m_processedHeight != m_image->height)
    return;

  glBindFramebuffer(GL_FRAMEBUFFER, m_processingFramebuffer);
  glViewport(0, 0, m_processedWidth, m_processedHeight);

  m_imageProcessingShader.use();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_image->textureId);
  glUniform1f(glGetUniformLocation(m_imageProcessingShader.ID, "exposure"),
              m_exposure);
  glUniform3fv(glGetUniformLocation(m_imageProcessingShader.ID, "whiteBalance"),
               1, m_color);

  glBindVertexArray(m_processingVao);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUseProgram(0);

  m_processingDirty = false;
}

void App::destroyImageProcessing() {
  if (m_imageProcessingShader.ID != 0)
    glDeleteProgram(m_imageProcessingShader.ID);
  if (m_processedTexture != 0)
    glDeleteTextures(1, &m_processedTexture);
  if (m_processingFramebuffer != 0)
    glDeleteFramebuffers(1, &m_processingFramebuffer);
  if (m_processingVao != 0)
    glDeleteVertexArrays(1, &m_processingVao);

  m_imageProcessingShader.ID = 0;
  m_processedTexture = 0;
  m_processingFramebuffer = 0;
  m_processingVao = 0;
  m_processingReady = false;
}

void App::run() {
  while (!glfwWindowShouldClose(m_window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    renderUI();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
  }
}

void App::shutdown() {
  for (auto &future : m_loadFutures) {
    if (future.valid())
      future.wait();
  }

  clearImage();
  destroyImageProcessing();
  newTriangle.destroy();

  // Destroying context and data freeing up memory
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

void App::renderUI() {
  // where to render different ui spaces
  renderDockSpace();
  renderMenuBar();
  photoViewer();
  renderDevelopPanel();
  filmStrip();
}

// Saving app settings. Last used dir for example
//
void App::registerSettingsHandler() {
  ImGuiSettingsHandler IniHandler;
  IniHandler.TypeName = "PhotoCrispy";
  IniHandler.TypeHash = ImHashStr("PhotoCrispy");
  IniHandler.UserData = this;
  IniHandler.ReadOpenFn = [](ImGuiContext *, ImGuiSettingsHandler *,
                             const char *) -> void * { return (void *)1; };
  IniHandler.ReadLineFn = [](ImGuiContext *, ImGuiSettingsHandler *h, void *,
                             const char *line) {
    App *app = (App *)h->UserData;
    char buf[512];
    if (std::sscanf(line, "LastDir=%511[^\n]", buf) == 1)
      app->m_lastDir = buf;
  };
  IniHandler.WriteAllFn = [](ImGuiContext *, ImGuiSettingsHandler *h,
                             ImGuiTextBuffer *buf) {
    App *app = (App *)h->UserData;
    buf->appendf("[PhotoCrispy][Settings]\nLastDir=%s\n",
                 app->m_lastDir.c_str());
  };
  ImGui::AddSettingsHandler(&IniHandler);
}

void App::openNewFile(const fs::path &path) {
  // Function to open File. Async. Push to queue
  const std::string filePathName = path.string();
  const fs::path directory = path.parent_path();

  if (directory != m_filmstripDir && browser.Refresh(directory)) {
    m_filmstripDir = directory;
  }

  m_lastDir = directory.string();
  ImGui::MarkIniSettingsDirty();
  const uint64_t generation = ++m_loadGeneration;
  m_loading = true;
  m_showingPreview = false;

  m_loadFutures.push_back(
      std::async(std::launch::async, [this, filePathName, generation]() {
        if (auto preview = extractEmbeddedPreviewImage(filePathName)) {
          m_loadResults.push(LoadResult{generation, std::move(*preview)});
        }
        if (auto full = decodeFullRawImage(filePathName)) {
          m_loadResults.push(LoadResult{generation, std::move(*full)});
        }
        m_completedLoads.push(generation);
      }));
}

void App::drawAboutWindow() {}

void App::renderMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Open")) {
        /* handle open - ImguiFileDialog */
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.path = m_lastDir;
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseFileDlgKey", "Choose File", ".ARW,.DNG,.RAF", config);
      }

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About")) {
        m_showAboutWindow = true;
      }

      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  if (m_showAboutWindow) {
    ImGui::SetNextWindowSize(ImVec2(400.0f, 220.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("About PhotoCrispy", &m_showAboutWindow)) {
      ImGui::TextUnformatted("PhotoCrispy");
      ImGui::Separator();

      ImGui::Text("Version: 0.1.0");
      ImGui::TextWrapped("A RAW image processing application built with "
                         "C++, OpenGL, Dear ImGui and LibRaw.");

      ImGui::Spacing();
      // Triangle in about. Opengl test
      ImVec2 triangleSize = ImGui::GetContentRegionAvail();
      triangleSize.y -= ImGui::GetFrameHeightWithSpacing();

      if (triangleSize.x > 0.0f && triangleSize.y > 0.0f) {
        newTriangle.renderTriangle(static_cast<int>(triangleSize.x),
                                   static_cast<int>(triangleSize.y));
        ImGui::Image(static_cast<ImTextureID>(newTriangle.texture()),
                     triangleSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
      }

      if (ImGui::Button("Close", ImVec2(100.0f, 0.0f))) {
        m_showAboutWindow = false;
      }
    }

    ImGui::End();
  }

  // ImGuiFileDialog Open
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
    if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      openNewFile(filePathName);
    }
    // close
    ImGuiFileDialog::Instance()->Close();
  }
}

void App::renderDevelopPanel() {
  ImGui::Begin("Navigation");
  // Zoom controls below the canvas
  if (m_image.has_value()) {
    ImGui::SliderFloat("Zoom", &m_zoom, 0.5f, 8.0f);
    ImGui::Text("Zoom: %.0f%%", m_zoom * 100.0f);
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset"))
      m_zoom = 1.0f;
    ImGui::TextDisabled(
        "Enable zoom with Ctrl\nCtrl + Alt for precise zooming.");
  }
  ImGui::End();
  ImGui::Begin("Develop Settings");
  ImGui::Text("Basic Adjustments");
  bool adjustmentsChanged = false;
  adjustmentsChanged |=
      ImGui::SliderFloat("Exposure", &m_exposure, -5.0f, 5.0f);
  adjustmentsChanged |=
      ImGui::SliderFloat3("White Balance RGB", m_color, 0.0f, 2.0f);

  if (ImGui::Button("Reset Adjustments")) {
    m_exposure = 0.0f;
    m_color[0] = 1.0f;
    m_color[1] = 1.0f;
    m_color[2] = 1.0f;
    adjustmentsChanged = true;
  }

  if (adjustmentsChanged)
    m_processingDirty = true;

  if (ImGui::Button("Export DNG")) {
    fmt::print("Exporting at exposure: {}\n", m_exposure);
  }

  ImGui::End();
}

void App::photoViewer() {
  while (auto result = m_loadResults.tryPop()) {
    if (result->generation != m_loadGeneration) {
      continue;
    }

    clearImage();
    m_image = uploadTexture(result->image);
    // editing part
    resizeProcessedImage(m_image->width, m_image->height);
    m_processingDirty = true;

    m_showingPreview = result->image.kind == ImageKind::Preview;

    if (result->image.kind == ImageKind::Full) {
      m_loading = false;
      m_showingPreview = false;
    }
  }

  for (auto it = m_loadFutures.begin(); it != m_loadFutures.end();) {
    if (it->valid() &&
        it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      it->get();
      it = m_loadFutures.erase(it);
    } else {
      ++it;
    }
  }

  while (auto completedGeneration = m_completedLoads.tryPop()) {
    if (*completedGeneration == m_loadGeneration)
      m_loading = false;
  }

  ImGui::Begin("Viewer");
  processImage();
  // As imgui is constantly rendering, we ask if m_image has value.
  if (m_image.has_value()) {
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    if (m_loading && m_showingPreview) {
      ImGui::TextDisabled("Loading full RAW...");
      canvasSize = ImGui::GetContentRegionAvail();
    }

    // Compute fit-to-canvas size, then apply zoom
    float aspect = (float)m_image->width / (float)m_image->height;
    float fitW = canvasSize.x;
    float fitH = fitW / aspect;
    if (fitH > canvasSize.y) {
      fitH = canvasSize.y;
      fitW = fitH * aspect;
    }

    float w = fitW * m_zoom;
    float h = fitH * m_zoom;
    // Center image only when it's smaller than the canvas
    float offsetX = (w < canvasSize.x) ? (canvasSize.x - w) * 0.5f : 0.0f;
    float offsetY = (h < canvasSize.y) ? (canvasSize.y - h) * 0.5f : 0.0f;

    // Scrollable canvas
    ImGui::BeginChild("##canvas", canvasSize, false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    // Mouse wheel zooms while hovering the canvas
    // Ctrl+scroll zooms to mouse; plain scroll pans (handled by ImGui)
    if (ImGui::IsWindowHovered()) {
      ImGuiIO &io = ImGui::GetIO();
      float wheel = io.MouseWheel;
      if (wheel != 0.0f && io.KeyCtrl) {
        // Implementation of zoom where the mouse pointer is.
        // Get current mouse position relative to the scrolling content
        // ImGui::GetCursorScreenPos() marks the top-left of your drawing area
        ImVec2 origin = ImGui::GetCursorScreenPos();
        float mousePosX = io.MousePos.x - origin.x + ImGui::GetScrollX();
        float mousePosY = io.MousePos.y - origin.y + ImGui::GetScrollY();
        // Save the relative position (the "percentage" of the way across the
        // image)
        float relativeX = mousePosX / m_zoom;
        float relativeY = mousePosY / m_zoom;

        // Alt: fine zoom (1% per notch), default: 10% per notch
        // TODO : Adjust via options.
        float step = io.KeyAlt ? 0.01f : 0.1f;
        m_zoom = std::clamp(m_zoom + (wheel * step), 0.1f, 8.0f);

        ImGui::SetScrollX((relativeX * m_zoom) - (io.MousePos.x - origin.x));
        ImGui::SetScrollY((relativeY * m_zoom) - (io.MousePos.y - origin.y));

        // Prevent ImGui from also panning the child window on this frame
        // without it a zoom event would also trigger ImGui's built-in
        // scroll on the child window in the same frame.
        io.MouseWheel = 0.0f;
      }
    }

    ImGui::SetCursorPos(ImVec2(offsetX, offsetY));

    // ImGui::Image((ImTextureID)(uintptr_t)m_image->textureId, ImVec2(w, h));
    ImGui::Image((ImTextureID)(uintptr_t)(m_processingReady
                                              ? m_processedTexture
                                              : m_image->textureId),
                 ImVec2(w, h));

    ImGui::EndChild();
  } else if (m_loading) {
    ImVec2 region = ImGui::GetContentRegionAvail();
    float barW = region.x * 0.5f;
    ImGui::SetCursorPos(ImVec2((region.x - barW) * 0.5f, region.y * 0.5f));
    ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(barW, 0.0f),
                       "Loading...");
  } else {
    ImGui::TextDisabled("No image loaded. Use File > Open.");
  }

  ImGui::End();
}

void App::filmStrip() {
  ImGui::Begin("Filmstrip");
  const fs::path &files = browser.GetSelectedFile();

  if (m_lastDir != ".") {
    bool sel = browser.Draw();

    if (sel) {
      openNewFile(files);
    }
  }

  ImGui::End();
}

void App::clearImage() {
  if (!m_image.has_value())
    return;

  GLuint textureId = static_cast<GLuint>(m_image->textureId);
  glDeleteTextures(1, &textureId);
  m_image.reset();
}

void App::renderDockSpace() {
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags host_flags =
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_HorizontalScrollbar |
      ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("DockSpace", nullptr, host_flags);
  ImGui::PopStyleVar();

  ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0, 0),
                   ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::End();
}
