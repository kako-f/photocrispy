#pragma once
#include "raw_processing.h"
#include "threaded_load.h"
#include "opengl_shader.h"
#include "opengl_compute_shader.h"
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

        // photo opengl render
        unsigned int outputPhotoTexture,inputPhotoTexture;
        unsigned int FBO, VAO, VBO, EBO, histSSBO;
        PhotoShader *textureShader = nullptr;

        // Histogram data
        ComputeShader *histogramShader = nullptr;
        const int binNum = 128;
        const int histBufferSizeBytes = sizeof(GLuint) * binNum * 4; // R, G, B, L
        std::vector<GLuint> cpuHistogramData;

        // image modifications params
        float currentHighlights = 1.0f; // Initial brightness
        float currentShadows = 1.0f;
        float currentContrast = 0.0f;
        float currentExposure = 1.0f;
        float currentSaturation = 0.0f;

        // temp
        float shadowLow = 0.4f;
        float shadowHigh = 0.1f;

        // testing masking range.
        float hightlightLow = 0.1f;
        float hightlightHigh = 0.9f;
        bool hasTexture = false;

    public:
        // Mouse / keyboard Inputs
        void handleInput(const ImVec2 &imageSize, const ImVec2 &panelSize, const ImVec2 &imagePos);

        // opengl Texture
        void photopenglInit();
        bool photoFBO(int width, int height);
        void rescaleFBO(int width, int height);
        void photoOpenglCleanup();
        bool loadImage2(const RawProcessor::RawImageInfo &imageInfo);
        void render3(const ThreadLoader::ThreadedImageLoader &imageLoader);
        void checkGLError(const std::string &step);
        GLuint getPhotoTexture() const { return inputPhotoTexture; }
        int getImageWidth() const { return imageWidth;}
        int getImageHeight() const { return imageHeight;}

        // histogram data
        void photoHistogram();
        void computeHistogram(GLuint editedPhotoTextureID, int imageWidth, int imageHeight);
        void readHistogramData();
        std::vector<GLuint> getHistogramData() const { return cpuHistogramData; }
        int getBinNum() const { return binNum; }

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
            currentExposure = 1.0f;
            currentSaturation = 0.0f;

            shadowLow = 0.4f;
            shadowHigh = 0.1f;
            hightlightLow = 0.1f;
            hightlightHigh = 0.9f;
        }
    };

}
