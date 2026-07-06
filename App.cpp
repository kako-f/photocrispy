#include "App.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include "fmt/core.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>

bool App::init()
{
    // Setup

    if (!glfwInit())
        return false;

    m_window = glfwCreateWindow(1280, 720, "PhotoCrispy", NULL, NULL);

    if (!m_window)
        return false;

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    registerSettingsHandler();

    ImGuiIO &io = ImGui::GetIO();

    // Docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    return true;
}

void App::run()
{
    while (!glfwWindowShouldClose(m_window))
    {
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

void App::shutdown()
{
    if (m_loadFuture.valid())
        m_loadFuture.wait();

    clearImage();

    // Destroying context and data freeing up memory
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void App::renderUI()
{
    // where to render different ui spaces
    renderDockSpace();
    renderMenuBar();
    photoViewer();
    renderDevelopPanel();
}

void App::registerSettingsHandler() {
    ImGuiSettingsHandler IniHandler;
    IniHandler.TypeName = "PhotoCrispy";
    IniHandler.TypeHash = ImHashStr("PhotoCrispy");
    IniHandler.UserData = this;
    IniHandler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char*) -> void* { return (void*)1; };
    IniHandler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler* h, void*, const char* line) {
        App* app = (App*)h->UserData;
        char buf[512];
        if (sscanf_s(line, "LastDir=%511[^\n]", buf, (unsigned int)sizeof(buf)) == 1)
            app->m_lastDir = buf;
    };
    IniHandler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf) {
        App* app = (App*)h->UserData;
        buf->appendf("[PhotoCrispy][Settings]\nLastDir=%s\n", app->m_lastDir.c_str());
    };
    ImGui::AddSettingsHandler(&IniHandler);
}


void App::renderMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open"))
            {
                /* handle open - ImguiFileDialog */
                IGFD::FileDialogConfig config;
                config.countSelectionMax = 1;
                config.path = m_lastDir;
                ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".ARW,.raw,.dng,.RAF", config);
            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // ImGuiFileDialog Open

    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        { // action if OK
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            m_lastDir = ImGuiFileDialog::Instance()->GetCurrentPath();
            ImGui::MarkIniSettingsDirty();
            const uint64_t generation = ++m_loadGeneration;
            m_loading = true;
            m_showingPreview = false;

            m_loadFuture = std::async(std::launch::async, [this, filePathName, generation]() {
                if (auto preview = extractEmbeddedPreviewImage(filePathName)) {
                    m_loadResults.push(LoadResult{ generation, std::move(*preview) });
                }

                if (auto full = decodeFullRawImage(filePathName)) {
                    m_loadResults.push(LoadResult{ generation, std::move(*full) });
                }
            });
        }

        // close
        ImGuiFileDialog::Instance()->Close();
    }
}

void App::renderDevelopPanel()
{
    ImGui::Begin("Navigation");
    // Zoom controls below the canvas
    if (m_image.has_value())
    {
        ImGui::SliderFloat("Zoom", &m_zoom, 0.5f, 8.0f);
        ImGui::Text("Zoom: %.0f%%", m_zoom * 100.0f);
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset"))
            m_zoom = 1.0f;
        ImGui::TextDisabled("Enable zoom with Ctrl\nCtrl + Alt for precise zooming.");

    }
    ImGui::End();
    ImGui::Begin("Develop Settings");
    ImGui::Text("Basic Adjustments");
    ImGui::SliderFloat("Exposure", &m_exposure, -5.0f, 5.0f);
    ImGui::ColorEdit3("White Balance Tint", m_color);

    if (ImGui::Button("Export DNG"))
    {
        fmt::print("Exporting at exposure: {}\n", m_exposure);
    }

    ImGui::End();
}

void App::photoViewer()
{
    while (auto result = m_loadResults.tryPop()) {
        if (result->generation != m_loadGeneration) {
            continue;
        }

        clearImage();
        m_image = uploadTexture(result->image);
        m_showingPreview = result->image.kind == ImageKind::Preview;

        if (result->image.kind == ImageKind::Full) {
            m_loading = false;
            m_showingPreview = false;
        }
    }

    if (m_loading && m_loadFuture.valid() &&
        m_loadFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        m_loadFuture.get();
        if (!m_showingPreview)
            m_loading = false;
    }

    ImGui::Begin("Viewer");

    // As imgui is constantly rendering, we ask if m_image has value.
    if (m_image.has_value())
    {
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();

        if (m_loading && m_showingPreview) {
            ImGui::TextDisabled("Loading full RAW...");
            canvasSize = ImGui::GetContentRegionAvail();
        }

        // Compute fit-to-canvas size, then apply zoom
        float aspect = (float)m_image->width / (float)m_image->height;
        float fitW = canvasSize.x;
        float fitH = fitW / aspect;
        if (fitH > canvasSize.y)
        {
            fitH = canvasSize.y;
            fitW = fitH * aspect;
        }

        float w = fitW * m_zoom;
        float h = fitH * m_zoom;
        // Center image only when it's smaller than the canvas
        float offsetX = (w < canvasSize.x) ? (canvasSize.x - w) * 0.5f : 0.0f;
        float offsetY = (h < canvasSize.y) ? (canvasSize.y - h) * 0.5f : 0.0f;
        
        // Scrollable canvas
        ImGui::BeginChild("##canvas", canvasSize, false, ImGuiWindowFlags_HorizontalScrollbar);

        // Mouse wheel zooms while hovering the canvas
        // Ctrl+scroll zooms to mouse; plain scroll pans (handled by ImGui)
        if (ImGui::IsWindowHovered())
        {
            ImGuiIO &io = ImGui::GetIO();
            float wheel = io.MouseWheel;
            if (wheel != 0.0f && io.KeyCtrl)
            {
                // Implementation of zoom where the mouse pointer is.
                // Get current mouse position relative to the scrolling content
                // ImGui::GetCursorScreenPos() marks the top-left of your drawing area
                ImVec2 origin = ImGui::GetCursorScreenPos();
                float mousePosX = io.MousePos.x - origin.x + ImGui::GetScrollX();
                float mousePosY = io.MousePos.y - origin.y + ImGui::GetScrollY();
                // Save the relative position (the "percentage" of the way across the image)
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

        ImGui::Image((ImTextureID)(uintptr_t)m_image->textureId, ImVec2(w, h));

        ImGui::EndChild();
    }
    else if (m_loading)
    {
        ImVec2 region = ImGui::GetContentRegionAvail();
        float barW = region.x * 0.5f;
        ImGui::SetCursorPos(ImVec2((region.x - barW) * 0.5f, region.y * 0.5f));
        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(barW, 0.0f), "Loading...");
    }
    else
    {
        ImGui::TextDisabled("No image loaded. Use File > Open.");
    }

    ImGui::End();
}

void App::clearImage()
{
    if (!m_image.has_value())
        return;

    GLuint textureId = static_cast<GLuint>(m_image->textureId);
    glDeleteTextures(1, &textureId);
    m_image.reset();
}

void App::renderDockSpace()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_HorizontalScrollbar |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("DockSpace", nullptr, host_flags);
    ImGui::PopStyleVar();

    ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0, 0),
                     ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}
