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
        float currentBrightness = 1.0f; // Initial brightness
        float currentShadows = 1.0f; // Initial brightness
        float currentExposure = 1.0f;
        float currentSaturation = 1.0f;

        float fs = 1.0;
        float ls = 1.0;

        bool hasTexture = false;

    public:
        void handleInput(const ImVec2 &imageSize, const ImVec2 &panelSize, const ImVec2 &imagePos);

        void photopenglInit();

        bool photoFBO(float width, float height);

        void rescaleFBO(float width, float height);

        unsigned int photoOpenglGetTexture() const { return inputPhotoTexture; }

        void photoOpenglCleanup();

        float getZoom() const { return imageZoom; }
        float getBrightness() const { return currentBrightness; }
        float getShadows() const { return currentShadows; }
        
        float getLs() const { return ls; }
        float getFs() const { return fs; }

        bool getIfTexture() const { return hasTexture; }

        void setZoom(float newZoom);
        void setBrightness(float newBrightnessVal);
        void setShadows(float newShadowVal);

        void setfs(float newFs);
        void setls(float newLs);

        void resetView()
        {
            imageZoom = 1.0f;
            panningOffset = ImVec2(0.0f, 0.0f);
        }

        void resetImageModifications()
        {
            currentBrightness = 1.0f;
        }

        bool loadImage2(const RawProcessor::RawImageInfo &imageInfo);

        void render3(const ThreadLoader::ThreadedImageLoader &imageLoader);

        void checkGLError(const std::string &step);
    };

}
