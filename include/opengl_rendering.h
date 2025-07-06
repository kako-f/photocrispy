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
        unsigned int FBO;
        unsigned int textureColorBuffer;
        unsigned int RBO;
        unsigned int EBO;
        unsigned int VAO, VBO;

    public:
        void openglInit();

        PhotoShader *textureShader = nullptr;
        
        bool triangleInit(float texWidth, float texHeight);
        void triangleRender(const float width, const float height);
        void rescaleFBO(float width, float height);

        unsigned int triangleGetTexture() const { return textureColorBuffer; }
        void triangleCleanup();
    };

}