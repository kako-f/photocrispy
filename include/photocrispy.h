#pragma once
#include "imageviewer.h"
#include "raw_processing.h"
#include "threaded_load.h"
#include "content_browser.h"
#include "imageutils.h"
#include <string>

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

        // UI state variables as members
        std::string selectedFilePath;
        bool shouldUpdateTexture = false;
        bool needsHistogramUpdate = false;
        // Flag for graceful application exit
        bool shouldExitApp = false; 

        // Instances of other modules/classes that hold state
        RawProcessor::RawImageInfo rawInfo;
        // RawProcessor::RawImageInfo rawJpg;
        ThreadLoader::ThreadedImageLoader imageLoader;
        ContentBrowser::RawBrowser imageSelect;
        ImageUtils::HistogramData loadHistogram;

        // ImageViewer instance
        ImageProcessor::ImageViewer imageViewport;

    public:
        // Constructor
        PhotoCrispyApp();
        // Destructor if needed for cleanup
        ~PhotoCrispyApp();

        // Render Program UI
        void RenderUI();
        // Check if the app should exit
        bool VerifyExit() const { return shouldExitApp; }
    };

/*     PhotoCrispyApp::PhotoCrispyApp()
    {
    }

    PhotoCrispyApp::~PhotoCrispyApp()
    {
    } */
}