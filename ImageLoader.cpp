#include "ImageLoader.h"
#include "StbImageDecoder.h"
#include <GLFW/glfw3.h>
#include <libraw/libraw.h>

std::optional<ImageData> decodeFullRawImage(const std::string& path)
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

    ImageData result;
    result.width  = img->width;
    result.height = img->height;
    result.channels = 3;
    result.is16Bit = true;
    result.kind = ImageKind::Full;
    result.pixels16.assign(src, src + pixelCount);

    LibRaw::dcraw_clear_mem(img);
    return result;
}

std::optional<ImageData> extractEmbeddedPreviewImage(const std::string& path)
{
    LibRaw raw;

    if (raw.open_file(path.c_str()) != LIBRAW_SUCCESS)
        return std::nullopt;

    if (raw.unpack_thumb() != LIBRAW_SUCCESS)
        return std::nullopt;

    libraw_processed_image_t* thumb = raw.dcraw_make_mem_thumb();
    if (!thumb)
        return std::nullopt;

    std::optional<ImageData> result;

    if (thumb->type == LIBRAW_IMAGE_JPEG) {
        result = decodeJpegMemoryToRgb(thumb->data, thumb->data_size, ImageKind::Preview);
    } else if (thumb->type == LIBRAW_IMAGE_BITMAP && thumb->colors >= 3) {
        ImageData image;
        image.width = thumb->width;
        image.height = thumb->height;
        image.channels = 3;
        image.is16Bit = false;
        image.kind = ImageKind::Preview;

        const int sourceChannels = thumb->colors;
        const int pixelCount = thumb->width * thumb->height;
        image.pixels8.reserve(pixelCount * 3);

        for (int i = 0; i < pixelCount; ++i) {
            const int source = i * sourceChannels;
            image.pixels8.push_back(thumb->data[source + 0]);
            image.pixels8.push_back(thumb->data[source + 1]);
            image.pixels8.push_back(thumb->data[source + 2]);
        }

        result = std::move(image);
    }

    LibRaw::dcraw_clear_mem(thumb);
    return result;
}

RawImage uploadTexture(const ImageData& data)
{
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (data.is16Bit) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, data.width, data.height,
                     0, GL_RGB, GL_UNSIGNED_SHORT, data.pixels16.data());
    } else {
        GLenum format = data.channels == 4 ? GL_RGBA : GL_RGB;
        GLint internalFormat = data.channels == 4 ? GL_RGBA8 : GL_RGB8;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, data.width, data.height,
                     0, format, GL_UNSIGNED_BYTE, data.pixels8.data());
    }

    return RawImage{ (unsigned int)texId, data.width, data.height, data.kind };
}
