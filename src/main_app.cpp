#include "main_app.h"
#include "raw_processing.h"
#include "gl_photo_texture.h"
#include "imgui.h"
#include "nfd.hpp"
#include <fmt/core.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include "imageviewer.h"

namespace PhotoRaw
{

    // Forward declarations
    void RenderDockSpace();
    void RenderMenuBar();

    void RenderLeftInfoPanel(ImageProcessor::ImageViewer &imageViewport);

    void RenderImageViewPanel(ImageProcessor::ImageViewer &imageViewport);
    void RenderImagePreviewPanel();
    void RenderRightInfoPanel();

    GLuint createTextureFromImage(const RawProcessor::RawImageInfo &img);

    static std::string selectedFilePath;
    static bool shouldUpdateTexture = false;

    RawProcessor::RawImageInfo rawInfo;

    void RenderUI()
    {
        static ImageProcessor::ImageViewer imageViewport;

        RenderDockSpace();

        // Main menu bar
        RenderMenuBar();

        // left Info UI panel
        RenderLeftInfoPanel(imageViewport);

        // View Image Panel
        RenderImageViewPanel(imageViewport);

        // Image strip
        RenderImagePreviewPanel();

        // Right Info Panel
        RenderRightInfoPanel();

        // ImGui::ShowDemoWindow();
        // ImGui::ShowMetricsWindow();
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
                        rawInfo = RawProcessor::loadRaw(selectedFilePath);
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
    // Information of the current image is shown when its loaded.
    // An instance of ImageViewer is passed to access the associated 
    // methods of zoom and reset (and others...)
    void RenderLeftInfoPanel(ImageProcessor::ImageViewer &imageViewport)
    {
        ImGui::Begin("Image Info");
        if (rawInfo.success)
        {
  
            std::string message = fmt::format("Opened file path: {}", selectedFilePath);
            ImGui::TextWrapped("%s", message.c_str());
            ImGui::TextWrapped("Image Size: %d x %d", rawInfo.width, rawInfo.height);
            ImGui::TextWrapped("Camera make: %s", rawInfo.cam_make.c_str());
            ImGui::TextWrapped("Camera model: %s", rawInfo.cam_model.c_str());
            ImGui::Separator();
            // from 10% to 1000%
            float zoom = imageViewport.getZoom() * 100;
            if (ImGui::SliderFloat("Zoom", &zoom, 10.0f, 1000.0f, "%.0f%%"))
            {
                imageViewport.setZoom(zoom/ 100.0f); // update class value if slider changed
            }

            //ImGui::SliderFloat("Zoom", &zoom, 10.0f, 1000.0f, "%.0f%%");


            ImGui::Text("Zoom Level: %.0f%%", imageViewport.getZoom() * 100.0f);

            if (ImGui::Button("Reset View"))
            {
                imageViewport.resetView();
            }
        }

        ImGui::End();
    }
    // An instance of ImageViewer is passed to render the image in the viewport
    // using gl_photo_texture for the moment
    // DX or Vulkan (or both) in the future.
    void RenderImageViewPanel(ImageProcessor::ImageViewer &imageViewport)
    {

        imageViewport.render();

        if (rawInfo.success && shouldUpdateTexture)
        {
            imageViewport.loadImage(rawInfo);
            shouldUpdateTexture = false;
        }
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