#pragma once

#include <glad/glad.h>

class TriangleRenderer {
public:
  void createTriangle();
  void renderTriangle();
  void createFrambuffer();
  void bind_framebuffer();
  void unbind_framebuffer();
  void rescale_framebuffer(float width, float height);

private:
  GLuint VAO;
  GLuint VBO;
  GLuint FBO;
  GLuint RBO;
};