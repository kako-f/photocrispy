#include "photocrispy.h"
#include "raw_processing.h"
#include "imageviewer.h"
#include "threaded_load.h"
#include "content_browser.h"
#include "imageutils.h"
#include "imgui.h"
#include "nfd.hpp"
#include <fmt/core.h>

#include <algorithm>

namespace PhotoCrispy
{
    // --- PhotoCrispyApp Class Implementation ---
    PhotoCrispyApp::PhotoCrispyApp(GLFWwindow *window)
        // Initialize member variables in the constructor's
        : currentWindow(window),
          selectedFilePath(""),
          shouldUpdateTexture(false),
          needsHistogramUpdate(false),
          shouldExitApp(false),
          noClose(false),
          rawInfo(), // Constructor for RawImageInfo
          // rawJpg(),        //Constructor for RawImageInfo
          imageLoader(),   // Constructor for ThreadedImageLoader
          imageSelect(),   // Constructor for RawBrowser
          loadHistogram(), // Constructor for HistogramData
          imageViewport(), // Constructor for ImageViewer
          triangleRender() // Constructor for OpenglRendering - About Window
    {
        // Any complex initialization can go here, but simple member initialization
        // is best done in the initializer list above.
        // For example, if ImageViewer needed parameters, they'd go there.

        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        // fmt::print("{} {}", fb_height, fb_width);

        triangleRender.triangleInit((float)fb_height, (float)fb_width);

        imageViewport.photoFBO((float)fb_height, (float)fb_width);

        fmt::print("PhotoCrispyApp initialized.\n");
    }
    PhotoCrispyApp::~PhotoCrispyApp()
    {
        // Clean up any resources held by the class if necessary
        triangleRender.triangleCleanup();
        imageViewport.photoOpenglCleanup();
        fmt::print("PhotoCrispyApp shutting down.\n");
    }
    void PhotoCrispyApp::renderUI()
    {
        RenderDockSpace();
        RenderMenuBar();
        RenderLeftInfoPanel();
        RenderImageViewPanel();
        RenderImagePreviewPanel();
        RenderRightInfoPanel();

        ImGui::ShowDemoWindow();
        ImGui::ShowMetricsWindow();
    }

    void PhotoCrispyApp::RenderDockSpace()
    {
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    }

    void PhotoCrispyApp::RenderMenuBar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open"))
                {
                    // NFD Dialog
                    NFD::Guard nfdGuard; // NFD::Guard will automatically quit NFD.
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
                        fmt::print("Success!! {}\n", selectedFilePath); // fmt can directly print std::string
                        imageLoader.startAsyncLoad(selectedFilePath);
                        imageViewport.resetImageModifications();
                    }
                    else if (result == NFD_CANCEL)
                    {
                        fmt::print("User pressed cancel.\n");
                    }
                    else
                    {
                        fmt::print("Error: {}\n", NFD::GetError());
                    }
                }
                if (ImGui::MenuItem("Exit"))
                {
                    // Flag for exit
                    shouldExitApp = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::MenuItem("Undo");
                ImGui::MenuItem("Redo");
                ImGui::MenuItem("Patito"); // Placeholder/Test item
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("About"))
            {
                if (ImGui::MenuItem("About PhotoCrispy"))
                {
                    noClose = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
        if (imageLoader.pollLoadResult(rawInfo))
        {
            shouldUpdateTexture = true;
            loadHistogram = ImageUtils::calculateHistogram(rawInfo);
            needsHistogramUpdate = true;
        }
        if (noClose)
        {

            // we need to check orientation.
            // ImGui uses top-left vs OpenGL bottom-left
            // we flip it with
            // uv0 = (0,0) → top-left
            // uv1 = (1,1) → bottom-right
            // third and fouth argument of Image
            // ImVec2(window_width, window_height)
            // triangleRender.triangleGetTexture()
            ImGui::Begin("Triangle Window", &noClose);

            const float window_width = ImGui::GetContentRegionAvail().x;
            const float window_height = ImGui::GetContentRegionAvail().y;

            fmt::print("w: {}, h: {}\n", window_width, window_height);
            // Render triangle to FBO texture
            triangleRender.triangleRender(window_width, window_height);
            ImGui::Text("Triangle rendered to FBO texture:");
            ImGui::Image(
                (intptr_t)triangleRender.triangleGetTexture(),
                ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::End();
        }
    }

    void PhotoCrispyApp::RenderLeftInfoPanel()
    {
        ImGui::Begin("Image Information");
        if (rawInfo.success)
        {
            ImGui::TextWrapped("Opened file path: %s", selectedFilePath.c_str());
            ImGui::TextWrapped("Image Size: %d x %d", rawInfo.width, rawInfo.height);
            ImGui::TextWrapped("Camera make: %s", rawInfo.cam_make.c_str());
            ImGui::TextWrapped("Lense: %s", rawInfo.lens_model.c_str());
            ImGui::Separator();
            // from 10% to 1000%
            float zoom = imageViewport.getZoom() * 100;
            if (ImGui::SliderFloat("Zoom", &zoom, 10.0f, 1000.0f, "%.0f%%"))
            {
                // update class value if slider changed
                imageViewport.setZoom(zoom / 100.0f);
            }
            ImGui::Text("Zoom Level: %.0f%%", imageViewport.getZoom() * 100.0f);

            if (ImGui::Button("Reset View"))
            {
                imageViewport.resetView();
            }
            ImGui::Separator();
        }
        ImGui::End();
    }

    void PhotoCrispyApp::RenderImagePreviewPanel()
    {
        imageSelect.browseImages();
    }

    void PhotoCrispyApp::RenderImageViewPanel()
    {
        /*         ImGui::Begin("Photo Window");

                const float window_width = ImGui::GetContentRegionAvail().x;
                const float window_height = ImGui::GetContentRegionAvail().y;

                fmt::print("w: {}, h: {}\n", window_width, window_height);
                // Render triangle to FBO texture
                imageViewport.render2(window_height,window_height);
                ImGui::Text("patito:");
                ImGui::Image(
                    (intptr_t)imageViewport.photoOpenglGetTexture(),
                    ImVec2(window_width, window_height), ImVec2(0, 1), ImVec2(1, 0));

                ImGui::End(); */

        // passing the member image loader

        if (rawInfo.success && shouldUpdateTexture)
        {
            imageViewport.loadImage2(rawInfo);
            shouldUpdateTexture = false;
        }

        imageViewport.render3(imageLoader);
    }

    void PhotoCrispyApp::RenderRightInfoPanel()
    {
        ImGui::Begin("rightInfo");
        if (rawInfo.success || needsHistogramUpdate)
        {
            // Reset the flag once the histogram is about to be rendered/updated
            needsHistogramUpdate = false;

            if (!loadHistogram.red.empty())
            {
                float graph_width = ImGui::GetContentRegionAvail().x;
                float graph_height = 125.0f;

                ImageUtils::drawOverlayedHistogram(loadHistogram, graph_width, graph_height);

                // Optional: Add a legend or labels below the plot
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "L");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "R");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "G");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "B");
            }
        }
        if (rawInfo.success)
        {
            ImGui::SeparatorText("Balance/Tint");
            ImGui::SeparatorText("Tone");
            // we need to reset the values when loading a new pict
            float currentExposure = imageViewport.getExposure();
            if (ImGui::SliderFloat("Exposure", &currentExposure, -4.0f, 4.0f, "%.4f"))
            {
                // update class value if slider changed
                imageViewport.setExposure(currentExposure);
            }
            float currentContrast = imageViewport.getContrast();
            if (ImGui::SliderFloat("Contrast", &currentContrast, -1.0f, 1.0f, "%.4f"))
            {
                imageViewport.setContrast(currentContrast);
            }
            ImGui::Separator();
            float currentHighlights = imageViewport.getHighlights();
            if (ImGui::SliderFloat("Highlights", &currentHighlights, -1.0f, 2.0f, "%.4f"))
            {
                // update class value if slider changed
                imageViewport.setHighlights(currentHighlights);
            }
            float currentShadows = imageViewport.getShadows();
            if (ImGui::SliderFloat("Shadows", &currentShadows, -1.0f, 2.0f, "%.4f"))
            {
                // update class value if slider changed
                imageViewport.setShadows(currentShadows);
            }
            ImGui::SeparatorText("Color");
            float currentSaturation = imageViewport.getSaturation();
            if (ImGui::SliderFloat("Saturation", &currentSaturation, -1.0f, 1.0f, "%.3f"))
            {
                // update class value if slider changed
                imageViewport.setSaturation(currentSaturation);
            }
            ImGui::Separator();
            float pstep = 0.005f;
            float fstep = 0.05f;
            float currentShadowLow = imageViewport.getShadowLow();
            float currentShadowHigh = imageViewport.getShadowHigh();
            static float shadowThresh[2] = {currentShadowLow, currentShadowHigh};
            if (ImGui::InputScalarN("Shadows High/Low Falloff", ImGuiDataType_Float, shadowThresh, 2, &pstep, &fstep, "%.3f"))
            {
                currentShadowLow = shadowThresh[0];
                currentShadowHigh = shadowThresh[1];
                imageViewport.setShadowLow(currentShadowLow);
                imageViewport.setShadowHigh(currentShadowHigh);
            }
            ImGui::Separator();
            float currentHighlightsLow = imageViewport.getHighlightsLow();
            float currentHighlightsHigh = imageViewport.getHighlightsHigh();
            static float highlightThresh[2] = {currentHighlightsLow, currentHighlightsHigh};
            if (ImGui::InputScalarN("Highlights High/Low Falloff", ImGuiDataType_Float, highlightThresh, 2, &pstep, &fstep, "%.3f"))
            {
                currentHighlightsLow = highlightThresh[0];
                currentHighlightsHigh = highlightThresh[1];
                imageViewport.setHighlightsLow(currentHighlightsLow);
                imageViewport.setHighlightsHigh(currentHighlightsHigh);
            }
            if (ImGui::Button("Reset"))
            {
                imageViewport.resetImageModifications();
            }
        }

        ImGui::End();
    }

}
