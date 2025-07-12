#include "imageviewer.h"

namespace ImageProcessor
{
    void ImageViewer::photoOpenglCleanup()
    {
        glDeleteFramebuffers(1, &FBO);
        glDeleteTextures(1, &outputPhotoTexture);
        glDeleteRenderbuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(textureShader->ID);
    }

    void ImageViewer::rescaleFBO(float width, float height)
    {
        glBindTexture(GL_TEXTURE_2D, outputPhotoTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width, (GLsizei)height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputPhotoTexture, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, EBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)width, (GLsizei)height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, EBO);
    }

    // Helper to check for OpenGL errors
    void ImageViewer::checkGLError(const std::string &step)
    {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            fmt::print("OpenGL Error at step [{}], {}", step, err);
        }
    }

    void ImageViewer::photopenglInit()
    {
        float vertices[] = {
            // positions        // texture coords
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // top right
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // top left
        };

        unsigned int indices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        checkGLError("Gen Buffers");

        glBindVertexArray(VAO);
        checkGLError("Bind VAO (setup)");

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        checkGLError("Buffer Data VBO");

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        checkGLError("Buffer Data EBO");

        // Configure vertex attributes:
        // Position attribute (layout (location = 0))
        // 3 floats per position, offset 0, stride 5 floats (3 pos + 2 tex)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        checkGLError("Vertex Attrib Pos");

        // Texture coordinate attribute (layout (location = 1))
        // 2 floats per tex coord, offset 3 floats (after x,y,z), stride 5 floats
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        checkGLError("Vertex Attrib TexCoord");

        // Unbind VAO and VBO (important to unbind VAO last)
        // not sure if needed
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        checkGLError("Unbind Buffers");
        textureShader = new PhotoShader("../../include/shaders/photo.vert", "../../include/shaders/photo.frag");

        checkGLError("Shader Creation");
    }

    bool ImageViewer::photoFBO(float width, float height)
    {

        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glGenTextures(1, &outputPhotoTexture);
        glBindTexture(GL_TEXTURE_2D, outputPhotoTexture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width, (GLsizei)height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputPhotoTexture, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glGenRenderbuffers(1, &EBO);
        glBindRenderbuffer(GL_RENDERBUFFER, EBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)width, (GLsizei)height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, EBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            fmt::print("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // glBindTexture(GL_TEXTURE_2D, 0);

        /*         glBindRenderbuffer(GL_RENDERBUFFER, 0); */

        photopenglInit();

        return true;
    }

    bool ImageViewer::loadImage2(const RawProcessor::RawImageInfo &imageInfo)
    {
        imageWidth = imageInfo.width;
        imageHeight = imageInfo.height;
        imageColors = imageInfo.colors;
        imageBits = imageInfo.bits;

        imageZoom = 1.0f;
        panningOffset = ImVec2(0, 0);

        const unsigned char *data = imageInfo.data.data();

        GLenum format = GL_RGB;
        if (imageBits == 1)
            format = GL_RED;
        else if (imageBits == 3)
            format = GL_RGB;
        else if (imageBits == 4)
            format = GL_RGBA;

        glGenTextures(1, &inputPhotoTexture);
        glBindTexture(GL_TEXTURE_2D, inputPhotoTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, imageWidth, imageHeight, 0, format, GL_UNSIGNED_BYTE, data);
        // Setup filtering parameters for display
        // GL_NEAREST / More pixelated "zoomed" image.
        // GL_LINEAR / smoothed max zoom, less pixels
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        hasTexture = true;

        return true;
    }

    void ImageViewer::render3(const ThreadLoader::ThreadedImageLoader &imageLoader)
    {

        // No scrollbar to viewport.
        // TODO
        // Probably need to pass it as an option
        ImGuiWindowFlags windowsFlags = 0;
        windowsFlags |= ImGuiWindowFlags_NoScrollbar;

        ImGui::Begin("viewport", nullptr, windowsFlags);
        if (!hasTexture && !imageLoader.isLoading())
        {
            ImGui::Text("No image loaded.");
            ImGui::End();
            return;
        }
        else if (imageLoader.isLoading())
        {
            ImGui::Text("Loading %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
        }
        else if (hasTexture)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            rescaleFBO((float)imageWidth, (float)imageHeight);

            glViewport(0, 0, (GLsizei)imageWidth, (GLsizei)imageHeight);
            glClear(GL_COLOR_BUFFER_BIT);

            textureShader->use();
            glBindVertexArray(VAO);

            glActiveTexture(GL_TEXTURE0); // Activate texture unit 0
            glBindTexture(GL_TEXTURE_2D, inputPhotoTexture);

            textureShader->setInt("uTexture", 0);                           // Set texture unit to 0
            textureShader->setFloat("uHighlightFactor", currentBrightness); // Pass the brightness value

            textureShader->setFloat("fs", fs); // Pass the brightness value
            textureShader->setFloat("ls", ls); // Pass the brightness value

            // textureShader->setFloat("uShadowLift", currentShadows); // Pass the brightness value

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // 6 indices for 2 triangles

            glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to screen

            // Size of the current panel and position (coordinates) of the currentWindow
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();

            // Calculate aspect-ratio-preserving draw size
            // draw Height/Width are floats that are modified accordingly
            float imageAspect = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
            float panelAspect = avail.x / avail.y;
            float drawWidth, drawHeight;

            // Aspect Ratio
            if (panelAspect > imageAspect)
            {
                // Panel is wider — fit by height
                drawHeight = avail.y;
                drawWidth = drawHeight * imageAspect;
            }
            else
            {
                // Panel is narrower — fit by width
                drawWidth = avail.x;
                drawHeight = drawWidth / imageAspect;
            }

            // Apply zoom
            // Check handleInput for details
            drawWidth *= imageZoom;
            drawHeight *= imageZoom;

            // Set centered position of the image in the viewport
            // Remember that graphic data is rendered from the point (0, 0)
            // that is typically the top-left corner of the drawing surface (viewport).
            ImVec2 centeredPos = {
                cursorPos.x + (avail.x - drawWidth) * 0.5f + panningOffset.x,
                cursorPos.y + (avail.y - drawHeight) * 0.5f + panningOffset.y};

            ImGui::SetCursorScreenPos(centeredPos);

            ImGui::Image((ImTextureID)(intptr_t)outputPhotoTexture, ImVec2(drawWidth, drawHeight));
            handleInput(ImVec2(drawWidth, drawHeight), avail, centeredPos);
        }

        ImGui::End();
    }
    // Function to handle inputs to the image.
    // Panning and reset panning and zoom
    // imageSize = Width and Height (vector)
    // panelSize = available space in the viewport (vector)
    // imagePos = calculated center of the image in the viewport (vector)
    void ImageViewer::handleInput(const ImVec2 &imageSize, const ImVec2 &panelSize, const ImVec2 &imagePos)
    {
        // Input/Ouput ImGui
        ImGuiIO &io = ImGui::GetIO();
        // Panning of the image
        if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            panningOffset.x += io.MouseDelta.x;
            panningOffset.y += io.MouseDelta.y;
        }
        // Double Click reset
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            imageZoom = 1.0f;
            panningOffset = ImVec2(0.0f, 0.0f);
        }

        // Image Zoom
        // if Shift is hold, change to 10% zoom step
        // -----
        // Zoom is centered arround the mouse pointer

        float scroll = io.MouseWheel;
        if (ImGui::IsWindowHovered() && scroll != 0.0f)
        {
            // 1% zoom per step or 10% with shit
            float zoomFactor = io.KeyShift ? 1.1f : 1.01f;
            float previousZoom = imageZoom;

            // Scroll wheel movement.
            if (scroll > 0)
                imageZoom *= zoomFactor;
            else
                imageZoom /= zoomFactor;
            // Safe Margins 10% / 1000%
            imageZoom = std::clamp(imageZoom, 0.1f, 10.0f);

            // Capturing mouse position
            ImVec2 mousePos = io.MousePos;

            // Calculate relative position of mouse to image
            // imagePos is a vector of the image  in the viewport.
            // Top left corner is (0.0), from where the image is loaded
            ImVec2 mouseToImage = {
                mousePos.x - imagePos.x,
                mousePos.y - imagePos.y};

            // Normalized mouse offset in image (0 to 1)
            // relative coordinates ie middle of the image will be (.5,.5)
            ImVec2 mouseRatio = {
                mouseToImage.x / imageSize.x,
                mouseToImage.y / imageSize.y};

            // New image size according to the zoom

            float newWidth = imageSize.x * (imageZoom / previousZoom);
            float newHeight = imageSize.y * (imageZoom / previousZoom);

            // New image position after zoom
            ImVec2 newImagePos = {
                imagePos.x + (imageSize.x - newWidth) * 0.5f,
                imagePos.y + (imageSize.y - newHeight) * 0.5f};

            // New Target position of the same point under the mouse in the new image
            ImVec2 newMouseToImage = {
                mouseRatio.x * newWidth,
                mouseRatio.y * newHeight};
            // this is where the point under the cursor ends up after zooming, without any compensation
            ImVec2 newMouseImagePos = {
                newImagePos.x + newMouseToImage.x,
                newImagePos.y + newMouseToImage.y};

            // This adjustment pushes the image back so that the same image pixel remains under the cursor after the zoom.
            ImVec2 offsetChange = {
                mousePos.x - newMouseImagePos.x,
                mousePos.y - newMouseImagePos.y};
            // Passing the new panning values to the outside variables
            panningOffset.x += offsetChange.x;
            panningOffset.y += offsetChange.y;
        }
    }

    // Set Zoom Value
    void ImageViewer::setZoom(float newZoom)
    {
        // Clamp to safe range
        imageZoom = std::clamp(newZoom, 0.1f, 10.0f);
    }

    void ImageViewer::setBrightness(float newBrigthnessVal)
    {
        // Clamp to safe range
        currentBrightness = std::clamp(newBrigthnessVal, -1.0f, 2.0f);
    }
    void ImageViewer::setShadows(float newShadowVal)
    {
        // Clamp to safe range
        currentShadows = std::clamp(newShadowVal, -1.0f, 1.0f);
    }

    void ImageViewer::setfs(float newFs)
    {
        // Clamp to safe range
        fs = newFs;
    }

    void ImageViewer::setls(float newLs)
    {
        // Clamp to safe range
        ls = newLs;
    }
}
