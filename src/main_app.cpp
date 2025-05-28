#include "main_app.h"
#include "raw_processing.h"
#include "gl_photo_texture.h"
#include "imgui.h"
#include "nfd.hpp"
#include <fmt/core.h>
#include <GLFW/glfw3.h>
#include <algorithm>

namespace PhotoRaw
{

    // Forward declarations
    void RenderDockSpace();
    void RenderMenuBar();

    void RenderLeftInfoPanel();

    void RenderImageViewPanel();
    void RenderImagePreviewPanel();
    void RenderRightInfoPanel();

    GLuint createTextureFromImage(const RawProcessor::RawImageInfo &img);

    static std::string selectedFilePath;
    static bool shouldUpdateTexture = false;
    static gl_photo_texture photoRawTexture;
    static float imageZoom = 1.0f;

    RawProcessor::RawImageInfo info;

    void RenderUI()
    {
        RenderDockSpace();

        // Main menu bar
        RenderMenuBar();

        // left Info UI panel
        RenderLeftInfoPanel();

        // View Image Panel
        RenderImageViewPanel();

        // Image strip
        RenderImagePreviewPanel();

        // Right Info Panel
        RenderRightInfoPanel();

        // ImGui::ShowDemoWindow();
        ImGui::ShowMetricsWindow();
    }

    void RenderDockSpace()
    {
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    }

    void RenderMenuBar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open"))
                {

                    // NFD Dialog
                    // NFD::Guard will automatically quit NFD.
                    NFD::Guard nfdGuard;
                    // auto-freeing memory
                    NFD::UniquePath outPath;
                    // prepare filters for the dialog
                    nfdfilteritem_t filterItem[1] = {{"RAW Image Files", "dng,arw"}};
                    // show the dialog
                    // outPath is the path
                    // filterItem is the file types
                    // n is the quantity of file types
                    nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 1);
                    if (result == NFD_OKAY)
                    {
                        selectedFilePath = outPath.get();
                        std::string out_message = fmt::format("Success!! {}\n", outPath.get());
                        fmt::print("{}", out_message);
                        info = RawProcessor::loadRaw(selectedFilePath);
                        shouldUpdateTexture = true;
                    }
                    else if (result == NFD_CANCEL)
                    {
                        fmt::print("User pressed cancel.");
                    }
                    else
                    {
                        std::string error_message = fmt::format("Error {}", NFD::GetError());
                    }
                }
                if (ImGui::MenuItem("Exit"))
                {
                    // Handle exit action
                    exit(0);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::MenuItem("Undo");
                ImGui::MenuItem("Redo");
                ImGui::MenuItem("Patito");
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
        //
    }
    // Information of the current image is shown when this is loaded.
    void RenderLeftInfoPanel()
    {

        ImGui::Begin("File Info");
        if (info.success)
        {
            std::string message = fmt::format("Opened file path: {}", selectedFilePath);
            ImGui::TextWrapped("%s", message.c_str());
            ImGui::Text("Image Size: %d x %d", info.width, info.height);
            ImGui::Text("Camera make: %s", info.cam_make.c_str());
            ImGui::Text("Camera model: %s", info.cam_model.c_str());
            
            // from 10% to 500%
            ImGui::SliderFloat("Zoom", &imageZoom, 0.1f, 5.0f, "%.1fx");
            // Reset zoom. 
            if (ImGui::Button("Reset"))
            {
                imageZoom = 1.0f;
            }
        }

        ImGui::End();
    }

    void RenderImageViewPanel()
    {
        ImGui::Begin("viewport");

        if (info.success && shouldUpdateTexture)
        {
            photoRawTexture = gl_photo_texture();
            photoRawTexture.create_texture(info.width, info.height, info.colors, info.data.data());
            shouldUpdateTexture = false;
        }
        if (photoRawTexture.getID())
        {
            // Size of the current panel and position (coordinates) of the currentWindow
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 winCoords = ImGui::GetCursorPos();

            // Calculate aspect-ratio-preserving draw size
            // draw Height/Width are floats that are modified accordingly
            float imageAspect = static_cast<float>(info.width) / static_cast<float>(info.height);
            float panelAspect = avail.x / avail.y;
            float drawWidth, drawHeight;

            if (panelAspect > imageAspect)
            {
                // Panel is wider — fit by height
                drawHeight = avail.y;
                drawWidth = drawHeight * imageAspect;
            }
            else
            {
                // Panel is narrower — fit by width
                drawWidth = avail.x;
                drawHeight = drawWidth / imageAspect;
            }
            // TODO
            // if Shift is hold, change to 10% zoom step
            float scroll = ImGui::GetIO().MouseWheel;
            if (ImGui::IsWindowHovered() && scroll != 0.0f)
            {
                float zoomFactor = 1.01f; // 1% zoom per step
                if (scroll > 0)
                    imageZoom *= zoomFactor;
                else
                    imageZoom /= zoomFactor;

                imageZoom = std::clamp(imageZoom, 0.1f, 5.0f);
            }

            // Centering the image in the viewport
            ImVec2 centerOffset = {
                std::max(0.0f, (avail.x - drawWidth * imageZoom) * 0.5f),
                std::max(0.0f, (avail.y - drawHeight * imageZoom) * 0.5f)};
            ImGui::SetCursorPos(ImVec2(winCoords.x + centerOffset.x, winCoords.y + centerOffset.y));

            // Render scaled image / apply zoom
            ImGui::Image((ImTextureID)(intptr_t)photoRawTexture.getID(), ImVec2(drawWidth * imageZoom, drawHeight * imageZoom));
        }
        ImGui::End();
    }

    void RenderImagePreviewPanel()
    {
        ImGui::Begin("strip");
        ImGui::Button("test");
        ImGui::End();
    }
    void RenderRightInfoPanel()
    {
        ImGui::Begin("rightInfo");
        ImGui::Button("test");
        ImGui::End();
    }

}