#pragma once
#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <fmt/core.h>

class PhotoShader
{
private:
    // Utility function for checking shader compilation/linking errors.
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                fmt::print("ERROR::SHADER_COMPILATION_ERROR of type: {}, {}\n", type, infoLog);
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                fmt::print("ERROR::PROGRAM_LINKING_ERROR of type: {}, {}\n", type, infoLog);
            }
        }
    }

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
};
