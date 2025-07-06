#include "imageviewer.h"

namespace ImageProcessor
{
    void ImageViewer::loadImage(const RawProcessor::RawImageInfo &imageInfo)
    {
        // New GL texture
        imageTexture = gl_photo_texture();

        // is JPG or RAW ?
        // JPG (or others) is regarding thumbnails and thumbnail generation.
        if (imageInfo.is_raw_encoded)
        {
            // save original image and create a copy to processed
            originalImageData = imageInfo.data;
            imageTexture.create_texture(imageInfo.width, imageInfo.height, imageInfo.colors, imageInfo.data.data());
            fmt::print("image loaded in texture id {}\n", imageTexture.getID());
        }
        else if (imageInfo.is_jpeg_encoded)
        {
            imageTexture.create_texture(imageInfo.width, imageInfo.height, imageInfo.colors, imageInfo.jpgdata);
            stbi_image_free(imageInfo.jpgdata);
            originalImageData.clear();
        }
        else
        {
            // No image loaded or unsupported format
            originalImageData.clear();
        }

        imageWidth = imageInfo.width;
        imageHeight = imageInfo.height;
        imageColors = imageInfo.colors;
        imageBits = imageInfo.bits;

        imageZoom = 1.0f;
        panningOffset = ImVec2(0, 0);
        hasTexture = true;
    }

    void ImageViewer::render(const ThreadLoader::ThreadedImageLoader &imageLoader, GLFWwindow &window)
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

        // Render scaled image
        ImGui::Image((ImTextureID)(intptr_t)imageTexture.getID(), ImVec2(drawWidth, drawHeight));
        handleInput(ImVec2(drawWidth, drawHeight), avail, centeredPos);
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

    // Get zoom value
    float *ImageViewer::getZoomPointer()
    {
        return &imageZoom;
    }
    // Set Zoom Value
    void ImageViewer::setZoom(float newZoom)
    {
        // Clamp to safe range
        imageZoom = std::clamp(newZoom, 0.1f, 10.0f);
    }

}
