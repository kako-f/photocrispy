#pragma once
#include "gl_photo_texture.h"
#include "raw_processing.h"
#include "threaded_load.h"
#include "imgui.h"
#include "photo_shader.h"
#include <GLFW/glfw3.h>

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
        int imageBits = 0;

        bool hasTexture = false;

        void handleInput(const ImVec2 &imageSize, const ImVec2 &panelSize, const ImVec2 &imagePos);

        // To store unmodified image data
        std::vector<unsigned char> originalImageData;

        // Image modification parameters
        unsigned int VAO, VBO, EBO;
        PhotoShader *imageShader = nullptr; // Pointer to your new shader object
        float currentBrightness = 0.0f;     // Initial brightness, can be controlled by ImGui
        // --- FBO related members ---
        unsigned int fbo;        // Framebuffer Object ID
        unsigned int fboTexture; // Texture attached to the FBO (where our image will be rendered)
        unsigned int rbo;        // Renderbuffer Object ID (for depth/stencil, optional for 2D)
        int fboWidth, fboHeight; // Dimensions of the FBO texture

    public:
        float getZoom() const { return imageZoom; }
        bool getIfTexture() const { return hasTexture; }
        void setZoom(float newZoom);
        float *getZoomPointer();

        void resetView()
        {
            imageZoom = 1.0f;
            panningOffset = ImVec2(0.0f, 0.0f);
        }
        void loadImage(const RawProcessor::RawImageInfo &imageInfo);
        void render(const ThreadLoader::ThreadedImageLoader &imageLoader, GLFWwindow &window);

        void setupImageRenderingQuad();
        void cleanupImageRenderingQuad();

        // --- New FBO methods ---
        void setupFBO(int width, int height);
        void resizeFBO(int width, int height); // Call this if your image display size changes
        void cleanupFBO();
    };

}
