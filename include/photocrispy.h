#pragma once
#include "imageviewer.h"
#include "raw_processing.h"
#include "threaded_load.h"
#include "content_browser.h"
#include "imageutils.h"
#include "opengl_rendering.h"

#include <GLFW/glfw3.h>

namespace PhotoCrispy
{
    class PhotoCrispyApp
    {
    private:
        void RenderDockSpace();
        void RenderMenuBar();
        void RenderLeftInfoPanel();
        void RenderImageViewPanel();
        void RenderImagePreviewPanel();
        void RenderRightInfoPanel();
        // No need to pass variables to any of these functions
        // as they live in the app class, the functions can access to them
        // once they are initialized.

        // UI state variables as members
        std::string selectedFilePath;
        bool shouldUpdateTexture = false;
        bool needsHistogramUpdate = false;
        // Flag for graceful application exit
        bool shouldExitApp = false;
        bool noClose = false;

        GLFWwindow *currentWindow; // Store the GLFW window pointer

        // Instances of other modules/classes that hold state
        RawProcessor::RawImageInfo rawInfo;
        // RawProcessor::RawImageInfo rawJpg;
        ThreadLoader::ThreadedImageLoader imageLoader;
        ContentBrowser::RawBrowser imageSelect;
        ImageUtils::HistogramData loadHistogram;
        
        // About window
        OpenGlRendering::TriangleRendering triangleRender;

        // ImageViewer instance
        ImageProcessor::ImageViewer imageViewport;

    public:
        // Constructor
        PhotoCrispyApp(GLFWwindow *window);
        // Destructor if needed for cleanup
        ~PhotoCrispyApp();

        // OpenGL Triangle
        void closeTriangle();
        // Render Program UI
        void renderUI();
        // Check if the app should exit
        bool verifyExit() const { return shouldExitApp; }
    };

}