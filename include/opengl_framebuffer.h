#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>

namespace OpenglFrameBuffer
{
    class FrameBuffer
    {
    private:
        unsigned int fbo;
        unsigned int texture;
        unsigned int rbo;

    public:
        FrameBuffer(float width, float height);
        ~FrameBuffer();
        unsigned int getFrameTexture() const { return texture;}
        void rescaleFBO(float width, float height);
        void bindFBO() const;
        void unbindFBO() const;
    };
}