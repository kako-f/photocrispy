#include "opengl_compute_shader.h"

ComputeShader::ComputeShader(const char *computePath)
{
    // 1. Retrieve the compute shader source code from filePath
    std::string computeCode;
    std::ifstream cShaderFile;

    // ensure ifstream objects can throw exceptions
    cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        // open file
        cShaderFile.open(computePath);
        std::stringstream cShaderStream;
        // read file's buffer contents into streams
        cShaderStream << cShaderFile.rdbuf();
        // close file handlers
        cShaderFile.close();
        // convert stream into string
        computeCode = cShaderStream.str();
    }
    catch (std::ifstream::failure e)
    {
        // Replace with fmt::print if you prefer
        fmt::print("ERROR::COMPUTE_SHADER::FILE_NOT_SUCCESFULLY_READ: {}\n", e.what());
    }

    const char *cShaderCode = computeCode.c_str();
    unsigned int compute;

    // load compute shader
    compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, nullptr);
    glCompileShader(compute);
    checkCompileErrors(compute, "COMPUTE");

    // compute shader Program
    ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    fmt::print("Compute Shader Program ID: {}\n", ID);

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(compute);
}

void ComputeShader::use()
{
    glUseProgram(ID);
}
void ComputeShader::unbind()
{
    glUseProgram(0);
}