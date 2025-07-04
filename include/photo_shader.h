#pragma once
#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class PhotoShader
{
private:

public:
    // Read OpenGL program ID
    unsigned int ID;

    // Constructor to read and build the shader
    PhotoShader(const char *vertexPath, const char *fragmentPath);

    // to use the shader
    void use();

    // utility functions
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
};
