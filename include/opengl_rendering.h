#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

namespace OpenGlRendering
{
    class TriangleRendering
    {
    private:
        unsigned int FBO;
        unsigned int textureColorBuffer;
        unsigned int RBO;
        unsigned int EBO;
        unsigned int VAO, VBO, shaderProgram;
        int width, height;

    public:
        void openglInit();

        void readShaderSrc(const char *fname, std::vector<char> &buffer);
        unsigned int compileShader(const char *fname, GLenum shaderType); 
        unsigned int createShaderProgram(const char *vertexShaderPath, const char *fragmentShaderPath);

        void useProgram();
        
        bool triangleInit(int texWidth, int texHeight);
        void triangleRender(const float width, const float height);

        unsigned int triangleGetTexture() const { return textureColorBuffer; }
        void triangleCleanup();
    };

}