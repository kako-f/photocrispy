#pragma once

#include "gl_photo_texture.h"
#include "raw_processing.h"
#include "threaded_load.h"
#include "imgui.h"

namespace ImageProcessor
{
    class ImageViewer
    {
    private:
        gl_photo_texture imageTexture;
        ImVec2 panningOffset = ImVec2(0.0f, 0.0f);
        float imageZoom = 1.0f;

        int imageWidth = 0;
        int imageHeight = 0;
        int imageColors = 0;

        bool hasTexture = false;

        void handleInput(const ImVec2 &imageSize, const ImVec2 &panelSize, const ImVec2 &imagePos);

    public:
        float getZoom() const { return imageZoom; }
        void setZoom(float newZoom);    
        float *getZoomPointer();

        void resetView()
        {
            imageZoom = 1.0f;
            panningOffset = ImVec2(0.0f, 0.0f);
        }
        void loadImage(const RawProcessor::RawImageInfo &imageInfo);
        void render(const ThreadLoader::ThreadedImageLoader &imageLoader);
    };

}
