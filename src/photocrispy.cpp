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

#include <algorithm>

namespace PhotoCrispy
{
    // --- PhotoCrispyApp Class Implementation ---
    PhotoCrispyApp::PhotoCrispyApp(GLFWwindow* window)
        // Initialize member variables in the constructor's
        : m_window(window),
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
          triangleRender() // Constructor for OpenglRendering // TESTING
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
    void PhotoCrispyApp::initOpenglPhoto()
    {
        imageViewport.setupImageRenderingQuad();
    }
    void PhotoCrispyApp::closeOpenglPhoto()
    {
        imageViewport.cleanupImageRenderingQuad();
    }

    bool PhotoCrispyApp::initTriangle()
    {
        return triangleRender.triangleInit(512, 512);
    }
    void PhotoCrispyApp::closeTriangle()
    {
        triangleRender.triangleCleanup();
    }
    void PhotoCrispyApp::RenderUI()
    {
        RenderDockSpace();
        RenderMenuBar();
        RenderLeftInfoPanel();
        RenderImageViewPanel();
        RenderImagePreviewPanel();
        RenderRightInfoPanel();

        // ImGui::ShowDemoWindow();
        // ImGui::ShowMetricsWindow();
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
            const float window_width = ImGui::GetContentRegionAvail().x;
            const float window_height = ImGui::GetContentRegionAvail().y;
            
            // Render triangle to FBO texture
            triangleRender.triangleRender(window_width, window_height);
            // we need to check orientation. 
            // ImGui uses top-left vs OpenGL bottom-left
            // we flip it with 
            // uv0 = (0,0) → top-left 
            // uv1 = (1,1) → bottom-right
            // third and fouth argument of Image
            ImGui::Begin("Triangle Window", &noClose);
            ImGui::Text("Triangle rendered to FBO texture:");
            ImGui::Image(
                (intptr_t)triangleRender.triangleGetTexture(),
                ImVec2(window_width, window_height), ImVec2(0,1), ImVec2(1,0));
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
        // passing the member image loader
        imageViewport.render(imageLoader, *m_window); 

        if (rawInfo.success && shouldUpdateTexture)
        {
            imageViewport.loadImage(rawInfo);
            shouldUpdateTexture = false;
        }


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

        ImGui::End();
    }

}
