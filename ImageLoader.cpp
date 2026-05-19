#include "ImageLoader.h"
#include <GLFW/glfw3.h>
#include <libraw/libraw.h>

std::optional<RawImageData> decodeRawImage(const std::string& path)
{
    // When you get to the viewport rendering step, you'll want a simple GLSL shader that 
    // applies gamma (pow(color, 1.0/2.2)) at minimum — or a proper tone mapper later.

    LibRaw raw;
    raw.imgdata.params.output_bps = 16;

    if (raw.open_file(path.c_str()) != LIBRAW_SUCCESS) return std::nullopt;
    if (raw.unpack()                != LIBRAW_SUCCESS) return std::nullopt;
    if (raw.dcraw_process()         != LIBRAW_SUCCESS) return std::nullopt;

    libraw_processed_image_t* img = raw.dcraw_make_mem_image();
    if (!img) return std::nullopt;

    int pixelCount = img->width * img->height * 3;
    const uint16_t* src = reinterpret_cast<const uint16_t*>(img->data);

    RawImageData result;
    result.width  = img->width;
    result.height = img->height;
    result.pixels.assign(src, src + pixelCount);

    LibRaw::dcraw_clear_mem(img);
    return result;
}

RawImage uploadTexture(const RawImageData& data)
{
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, data.width, data.height,
                 0, GL_RGB, GL_UNSIGNED_SHORT, data.pixels.data());

    return RawImage{ (unsigned int)texId, data.width, data.height };
}
