#pragma once
#include <glad/glad.h>
#include <string>

// std::string (introduced in C++17) is a lightweight, non-owning reference
// to a string or a character sequence. It is essentially a pointer to existing
// character data paired with a size, designed to provide fast, read-only access
// without copying data.

class ShaderProgram {
public:
  unsigned int ID = 0;
  void create_shader(std::string vertexSource, std::string fragmentSource);

  void use();
  void setFloat(const std::string &name, float value) const;

private:
  void checkCompileErrors(unsigned int shader, std::string type);
};