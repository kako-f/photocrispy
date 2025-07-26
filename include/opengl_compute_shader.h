#pragma once
#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <fmt/core.h>

class ComputeShader
{
private:
    // Utility function for checking shader compilation/linking errors.
    void checkCompileErrors(GLuint shaderOrProgramID, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];

        if (type == "COMPUTE") // Check for compile status of the compute shader
        {
            glGetShaderiv(shaderOrProgramID, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shaderOrProgramID, 1024, nullptr, infoLog);
                std::cerr << "ERROR::COMPUTE_SHADER_COMPILATION_ERROR of ID " << shaderOrProgramID << ":\n"
                          << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
            else
            {
                std::cout << "INFO::COMPUTE_SHADER_COMPILATION_SUCCESS for ID " << shaderOrProgramID << std::endl;
            }
        }
        else if (type == "PROGRAM") // Check for link status of the program
        {
            glGetProgramiv(shaderOrProgramID, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shaderOrProgramID, 1024, nullptr, infoLog);
                std::cerr << "ERROR::PROGRAM_LINKING_ERROR of ID " << shaderOrProgramID << ":\n"
                          << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
            else
            {
                std::cout << "INFO::PROGRAM_LINKING_SUCCESS for ID " << shaderOrProgramID << std::endl;
            }
        }
        // You might also have "VERTEX" and "FRAGMENT" cases here for other shaders
    }

public:
    // Read OpenGL program ID
    unsigned int ID;

    // Constructor to read and build the shader
    ComputeShader(const char *computePath);

    // to use the shader
    void use();
    void unbind();

    // utility functions
};
