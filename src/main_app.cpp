#include "main_app.h"
#include "raw_processing.h"
#include "gl_photo_texture.h"
#include "imgui.h"
#include "nfd.hpp"
#include <fmt/core.h>
#include <GLFW/glfw3.h>

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
        }

        ImGui::End();
    }

    void RenderImageViewPanel()
    {
        ImGui::Begin("viewport");
        ImGui::Button("test");
        if (info.success && shouldUpdateTexture)
        {
            photoRawTexture = gl_photo_texture();
            photoRawTexture.create_texture(info.width, info.height, info.colors, info.data.data());
            shouldUpdateTexture = false;
        }
        if (photoRawTexture.getID())
        {
            // Get available size of the panel
            ImVec2 avail = ImGui::GetContentRegionAvail();

            // Compute aspect-ratio-preserving draw size
            float imageAspect = static_cast<float>(info.width) / static_cast<float>(info.height);
            
            float panelAspect = avail.x / avail.y;

            float drawWidth, drawHeight;

            if (panelAspect > imageAspect) {
                // Panel is wider — fit by height
                drawHeight = avail.y;
                drawWidth = drawHeight * imageAspect;
            } else {
                // Panel is narrower — fit by width
                drawWidth = avail.x;
                drawHeight = drawWidth / imageAspect;
            }

            // Render image scaled
            ImGui::Image(
                (ImTextureID)(intptr_t)photoRawTexture.getID(),
                ImVec2(drawWidth, drawHeight)
            );

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