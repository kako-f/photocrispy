#include "gl_photo_texture.h"
#include <GLFW/glfw3.h>

gl_photo_texture::gl_photo_texture()
{
    glGenTextures(1, &textureID);
}
gl_photo_texture::~gl_photo_texture()
{
    glDeleteTextures(1, &textureID);
}
// Move Constructor
gl_photo_texture::gl_photo_texture(gl_photo_texture &&other) noexcept
{
    textureID = other.textureID; // Take ownership of other's texture
    other.textureID = 0;         // Leave other in a safe state (no deletion in its destructor)
}
// Move Assignment operator
gl_photo_texture &gl_photo_texture::operator=(gl_photo_texture &&other) noexcept
{
    if (this != &other)
    {
        if (textureID)
        {
            glDeleteTextures(1, &textureID); // Clean up current texture
        }
        textureID = other.textureID; // Take ownership of the other texture
        other.textureID = 0;         // Safe state for the other.
    }
    return *this;
}

bool gl_photo_texture::create_texture(int width, int height, int channels, const unsigned char *data)
{
    if (!textureID) return false;
    // helper function to glBindTexture
    bind();

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    // --
    // passing the data from our (RawImageInfo) struct ( in raw_processing) to this function.
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );
    // helper function to glBindTexture
    unbind();
    
    return true;

}

void gl_photo_texture::bind() const {
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void gl_photo_texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}