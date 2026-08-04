#pragma once
#include "graphics/shaderProgram.h"
#include <glad/glad.h>

class TriangleRenderer {
public:
  void createTriangle();
  // void renderTriangle();
  void createFramebuffer();
  void renderTriangle(int width, int height);
  void destroy();
  GLuint texture() const { return texture_id; }
  // void bind_framebuffer();
  // void unbind_framebuffer();
  // void rescale_framebuffer(float width, float height);
  ShaderProgram appShaders;

private:
  GLuint VAO = 0;
  GLuint VBO = 0;
  GLuint FBO = 0;
  GLuint texture_id = 0;
  int width = 0;
  int height = 0;
};