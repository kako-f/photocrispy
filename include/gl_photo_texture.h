#pragma once
#include <GLFW/glfw3.h>

class gl_photo_texture
{
private:
    GLuint textureID = 0;

public:
    // constructor and deconstructor
    gl_photo_texture();
    ~gl_photo_texture();

    // Deleted copy constructor/assignment
    gl_photo_texture(const gl_photo_texture&) = delete;
    gl_photo_texture& operator=(const gl_photo_texture&) = delete;

    // Move constructor/assignment
    gl_photo_texture(gl_photo_texture&& other) noexcept;
    gl_photo_texture& operator=(gl_photo_texture&& other) noexcept;

    // Create from raw image data
    bool create_texture(int width, int height, int channels, const unsigned char* data);

    // Get the OpenGL texture ID
    GLuint getID() const { return textureID; }

    // Optional: bind manually
    void bind() const;
    void unbind() const;
};
