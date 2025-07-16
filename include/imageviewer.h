#pragma once
#include "raw_processing.h"
#include "threaded_load.h"
#include "opengl_shader.h"

#include "imgui.h"
#include "stb_image.h"

#include <fmt/core.h>

namespace ImageProcessor
{
    class ImageViewer
    {
    private:
        // image interaction values
        ImVec2 panningOffset = ImVec2(0.0f, 0.0f);
        float imageZoom = 1.0f;

        // image related data
        int imageWidth = 0;
        int imageHeight = 0;
        int imageColors = 0;
        int imageBits = 0;

        // To store unmodified image data
        std::vector<unsigned char> originalImageData;

        // photo opengl render
        unsigned int outputPhotoTexture;
        unsigned int FBO, VAO, VBO, EBO;
        GLuint inputPhotoTexture = 0;
        PhotoShader *textureShader = nullptr;

        // image modifications params
        float currentHighlights = 1.0f; // Initial brightness
        float currentShadows = 1.0f;
        float currentContrast = 0.0f;
        float currentExposure = 0.0f;
        float currentSaturation = 0.0f;

        // temp
        float shadowLow = 0.5f;
        float shadowHigh = 0.2f;

        float hightlightLow = 0.2f;
        float hightlightHigh = 0.5f;
        bool hasTexture = false;

    public:
        // Mouse / keyboard Inputs
        void handleInput(const ImVec2 &imageSize, const ImVec2 &panelSize, const ImVec2 &imagePos);

        // opengl Texture
        void photopenglInit();
        bool photoFBO(float width, float height);
        void rescaleFBO(float width, float height);
        void photoOpenglCleanup();
        bool loadImage2(const RawProcessor::RawImageInfo &imageInfo);
        void render3(const ThreadLoader::ThreadedImageLoader &imageLoader);
        void checkGLError(const std::string &step);

        // Image controls and modifications
        float getZoom() const { return imageZoom; }
        void setZoom(float newZoom);

        // Modifications
        void setHighlights(float newBrightnessVal);
        float getHighlights() const { return currentHighlights; }

        void setSaturation(float newSaturationValue);
        float getSaturation() const { return currentSaturation; }

        void setShadows(float newShadowVal);
        float getShadows() const { return currentShadows; }

        void setExposure(float newExposureVal);
        float getExposure() const { return currentExposure; }

        void setContrast(float newContrastVal);
        float getContrast() const { return currentContrast; }

        // temporal
        void setShadowLow(float newShadowLow);
        void setShadowHigh(float newShadowHigh);
        float getShadowLow() const { return shadowLow; }
        float getShadowHigh() const { return shadowHigh; }

        void setHighlightsLow(float newHighlightLow);
        void setHighlightsHigh(float newHighlightHigh);
        float getHighlightsLow() const { return hightlightLow; }
        float getHighlightsHigh() const { return hightlightHigh; }

        void resetView()
        {
            imageZoom = 1.0f;
            panningOffset = ImVec2(0.0f, 0.0f);
        }

        void resetImageModifications()
        {
            currentHighlights = 1.0f;
            currentShadows = 1.0f;
            currentContrast = 0.0f;
            currentExposure = 0.0f;
            currentSaturation = 0.0f;

            shadowLow = 0.5f;
            shadowHigh = 0.2f;
            hightlightLow = 0.2f;
            hightlightHigh = 0.5f;
        }
    };

}
