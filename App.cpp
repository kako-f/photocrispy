#include "App.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include <algorithm>
#include <chrono>
#include <iostream>

bool App::init()
{
    // Setup

    if (!glfwInit())
        return false;

    m_window = glfwCreateWindow(1280, 720, "RAW Processor Prototype", NULL, NULL);

    if (!m_window)
        return false;

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

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
    renderToolPanel();
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
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".ARW,.raw,.dng,.jpg", config);
            }

            if (ImGui::MenuItem("Export DNG"))
            {
                /* handle export */
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z"))
            {
                /* handle undo */
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
            // std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
            // async call to decodeRawImage
            m_loadFuture = std::async(std::launch::async, decodeRawImage, filePathName);
            m_loading = true;
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
        ImGui::Text("Zoom: %.0f%%", m_zoom * 100.0f);
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset"))
            m_zoom = 1.0f;
    }
    ImGui::End();
    ImGui::Begin("Develop Settings");
    ImGui::Text("Basic Adjustments");
    ImGui::SliderFloat("Exposure", &m_exposure, -5.0f, 5.0f);
    ImGui::ColorEdit3("White Balance Tint", m_color);

    if (ImGui::Button("Export DNG"))
    {
        std::cout << "Exporting at exposure: " << m_exposure << std::endl;
    }

    ImGui::End();
}

void App::renderToolPanel()
{
    ImGui::Begin("Tools");
    ImGui::Text("Tool Panel");
    if (ImGui::Button("Tool 1"))
    { /* ... */
    }
    if (ImGui::Button("Tool 2"))
    { /* ... */
    }

    ImGui::End();
}

void App::photoViewer()
{
    // Poll the background decode — if ready, upload to GPU on the main thread
    // ask if (1st) we are loading something,
    if (m_loading && m_loadFuture.valid() && m_loadFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        auto result = m_loadFuture.get();
        if (result.has_value())
            m_image = uploadTexture(*result);
        m_loading = false;
    }

    ImGui::Begin("Viewer");

    // As imgui is constantly rendering, we ask if m_image has value.
    if (m_loading)
    {
        ImGui::TextDisabled("Loading...");
    }
    else if (m_image.has_value())
    {

        // actual canvas size
        ImVec2 canvasSize = ImVec2(
            ImGui::GetContentRegionAvail().x,
            ImGui::GetContentRegionAvail().y);

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

        // Scrollable canvas
        ImGui::BeginChild("##canvas", canvasSize, false, ImGuiWindowFlags_HorizontalScrollbar);

        // Mouse wheel zooms while hovering the canvas
        if (ImGui::IsWindowHovered())
        {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f)
                m_zoom = std::clamp(m_zoom + wheel * 0.1f, 0.1f, 8.0f);
        }

        // Center image only when it's smaller than the canvas
        float offsetX = (w < canvasSize.x) ? (canvasSize.x - w) * 0.5f : 0.0f;
        float offsetY = (h < canvasSize.y) ? (canvasSize.y - h) * 0.5f : 0.0f;
        ImGui::SetCursorPos(ImVec2(offsetX, offsetY));

        ImGui::Image((ImTextureID)(uintptr_t)m_image->textureId, ImVec2(w, h));

        ImGui::EndChild();
    }
    else
    {
        ImGui::TextDisabled("No image loaded. Use File > Open.");
    }

    ImGui::End();
}

void App::renderDockSpace()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Flags, options for docking space
    // https://oprypin.github.io/crystal-imgui/ImGui/ImGuiWindowFlags.html
    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("DockSpace", nullptr, host_flags);
    ImGui::PopStyleVar();

    ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0, 0),
                     ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}