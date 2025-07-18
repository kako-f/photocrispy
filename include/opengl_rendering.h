#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "opengl_shader.h"


namespace OpenGlRendering
{
    class TriangleRendering
    {
    private:
        unsigned int textureColorBuffer;
        unsigned int FBO, VAO, VBO, RBO, EBO;
        PhotoShader *textureShader = nullptr;

    public:
        void openglInit();

        bool triangleInit(float texWidth, float texHeight);
        void triangleRender(const float width, const float height);
        void rescaleFBO(float width, float height);

        unsigned int triangleGetTexture() const { return textureColorBuffer; }
        void triangleCleanup();
    };

}