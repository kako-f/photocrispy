#include "photocrispy.h"
#include "raw_processing.h"
#include "gl_photo_texture.h"
#include "imageviewer.h"
#include "threaded_load.h"
#include "content_browser.h"
#include "imageutils.h"
#include "imgui.h"
#include "nfd.hpp"
#include <fmt/core.h>
#include <GLFW/glfw3.h>
#include <algorithm>

namespace PhotoCrispy
{
    // --- PhotoCrispyApp Class Implementation ---
    PhotoCrispyApp::PhotoCrispyApp()
        // Initialize member variables in the constructor's
        : selectedFilePath(""),
          shouldUpdateTexture(false),
          needsHistogramUpdate(false),
          shouldExitApp(false),
          rawInfo(), // Constructor for RawImageInfo
          // rawJpg(),        //Constructor for RawImageInfo
          imageLoader(),   // Constructor for ThreadedImageLoader
          imageSelect(),   // Constructor for RawBrowser
          loadHistogram(), // Constructor for HistogramData
          imageViewport()  // Constructor for ImageViewer
    {
        // Any complex initialization can go here, but simple member initialization
        // is best done in the initializer list above.
        // For example, if ImageViewer needed parameters, they'd go there.
        fmt::print("PhotoCrispyApp initialized.\n");
    }
    PhotoCrispyApp::~PhotoCrispyApp()
    {
        // Clean up any resources held by the class if necessary
        fmt::print("PhotoCrispyApp shutting down.\n");
    }
    void PhotoCrispyApp::RenderUI()
    {
        RenderDockSpace();
        RenderMenuBar();
        RenderLeftInfoPanel(); // Now accesses imageViewer directly
        RenderImageViewPanel();
        RenderImagePreviewPanel();
        RenderRightInfoPanel();

        // ImGui::ShowDemoWindow();
        // ImGui::ShowMetricsWindow();
    }

/*     bool PhotoCrispyApp::VerifyExit() const
    {
        return shouldExitApp;
    } */

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

            ImGui::EndMainMenuBar();
        }
        if (imageLoader.pollLoadResult(rawInfo))
        {
            shouldUpdateTexture = true;
            loadHistogram = ImageUtils::calculateHistogram(rawInfo);
            needsHistogramUpdate = true;
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
            ImGui::TextWrapped("Camera model: %s", rawInfo.cam_model.c_str());
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
        imageViewport.render(imageLoader); // Now passing the member image loader

        if (rawInfo.success && shouldUpdateTexture)
        {
            imageViewport.loadImage(rawInfo);
            shouldUpdateTexture = false;
        }

        /*
        // Re-evaluate if rawJpg functionality is needed.
        // If not, remove rawJpg member and this commented block.
        if (rawJpg.is_jpeg_encoded && shouldUpdateTexture)
        {
            imageViewer.loadImage(rawJpg);
            shouldUpdateTexture = false;
        }
        */
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
                float max_val = *std::max_element(loadHistogram.luminance.begin(), loadHistogram.luminance.end());
                ImGui::Text("Max Bin Value: %.2f", max_val); // Display max normalized value

                // Plot Luminance/Grayscale
                ImGui::Text("Luminance");
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White
                ImGui::PlotLines(
                    "##LuminanceHistogram",
                    loadHistogram.luminance.data(),
                    loadHistogram.luminance.size(),
                    0,                                            // Offset
                    NULL,                                         // Overlay text
                    0.0f,                                         // Scale min
                    1.0f,                                         // Scale max (normalized)
                    ImVec2(ImGui::GetContentRegionAvail().x, 100) // Size of the plot
                );
                ImGui::PopStyleColor();

                // Plot Red Channel
                ImGui::Text("Red Channel");
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red
                ImGui::PlotLines(
                    "##RedHistogram",
                    loadHistogram.red.data(),
                    loadHistogram.red.size(),
                    0,
                    NULL,
                    0.0f,
                    1.0f,
                    ImVec2(ImGui::GetContentRegionAvail().x, 100));
                ImGui::PopStyleColor();

                // Plot Green Channel
                ImGui::Text("Green Channel");
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
                ImGui::PlotLines(
                    "##GreenHistogram",
                    loadHistogram.green.data(),
                    loadHistogram.green.size(),
                    0,
                    NULL,
                    0.0f,
                    1.0f,
                    ImVec2(ImGui::GetContentRegionAvail().x, 100));
                ImGui::PopStyleColor();

                // Plot Blue Channel
                ImGui::Text("Blue Channel");
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 0.0f, 1.0f, 1.0f)); // Blue
                ImGui::PlotLines(
                    "##BlueHistogram",
                    loadHistogram.blue.data(),
                    loadHistogram.blue.size(),
                    0,
                    NULL,
                    0.0f,
                    1.0f,
                    ImVec2(ImGui::GetContentRegionAvail().x, 100));
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Text("No histogram data available.");
            }
        }
        ImGui::End();
    }

}
