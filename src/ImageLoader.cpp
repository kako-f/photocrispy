#include "ImageLoader.h"
#include "StbImageDecoder.h"
#include <GLFW/glfw3.h>
#include <libraw/libraw.h>

// ImageLoader does the following
// 1. Fully decode a RAW file into 16-bit image data.
// 2. Extract a fast embedded preview from a RAW file.
// 3. Upload either the image data to an OpenGL texture (either one).

// Loading the raw file
// A LibRaw object is created with output_bps = 16
// opens, unpacks, and processes the RAW file.
// std::nullopt is returned upon failure
std::optional<ImageData> decodeFullRawImage(const std::string &path)
{
    // TODO
    // Apply a simple GLSL shader that does gamma (pow(color, 1.0/2.2)) at minimum
    // or a proper tone mapper later.

    LibRaw raw;
    raw.imgdata.params.output_bps = 16;

    if (raw.open_file(path.c_str()) != LIBRAW_SUCCESS)
        return std::nullopt;
    if (raw.unpack() != LIBRAW_SUCCESS)
        return std::nullopt;
    if (raw.dcraw_process() != LIBRAW_SUCCESS)
        return std::nullopt;

    // In memory RGB Image
    libraw_processed_image_t *img = raw.dcraw_make_mem_image();
    if (!img)
        return std::nullopt;
    // 3 channels - 16 bit uint16_t
    // RGB - and width * height
    int pixelCount = img->width * img->height * 3;
    const uint16_t *src = reinterpret_cast<const uint16_t *>(img->data);

    // result - Full Raw
    ImageData result;
    result.width = img->width;
    result.height = img->height;
    result.channels = 3;
    result.is16Bit = true;
    result.kind = ImageKind::Full;
    result.pixels16.assign(src, src + pixelCount);

    // clearing the data
    LibRaw::dcraw_clear_mem(img);
    return result;
}

// Extracting embedded preview
// JPEG or Bitmap
// Using a LibRaw Object
// std::nullopt is returned upon failure
std::optional<ImageData> extractEmbeddedPreviewImage(const std::string &path)
{
    LibRaw raw;

    if (raw.open_file(path.c_str()) != LIBRAW_SUCCESS)
        return std::nullopt;

    if (raw.unpack_thumb() != LIBRAW_SUCCESS)
        return std::nullopt;

    libraw_processed_image_t *thumb = raw.dcraw_make_mem_thumb();
    if (!thumb)
        return std::nullopt;

    std::optional<ImageData> result;
    // If JPEG
    // compressed JPG is passed to decodeJpegMemoryToRGB
    // If Bitmap
    // the code manually copies the first three channels into pixels8, producing RGB 8-bit preview data.
    // ImageKind::Preview; and is16Bit = False
    if (thumb->type == LIBRAW_IMAGE_JPEG)
    {
        result = decodeJpegMemoryToRgb(thumb->data, thumb->data_size, ImageKind::Preview);
    }
    else if (thumb->type == LIBRAW_IMAGE_BITMAP && thumb->colors >= 3)
    {
        ImageData image;
        image.width = thumb->width;
        image.height = thumb->height;
        image.channels = 3;
        image.is16Bit = false;
        image.kind = ImageKind::Preview;

        const int sourceChannels = thumb->colors;
        const int pixelCount = thumb->width * thumb->height;
        image.pixels8.reserve(pixelCount * 3);

        for (int i = 0; i < pixelCount; ++i)
        {
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
// Creates and binds a texture, sets linear filtering, then chooses
// the upload format based on data.is16Bit.
// For full RAW data, it uploads as a 16-bit RGB texture:
//
// GL_RGB16
// GL_UNSIGNED_SHORT
// data.pixels16.data()
//
// For preview data, it uploads as an 8-bit RGB/RGBA texture:
//
// GL_RGB8 or GL_RGBA8
// GL_UNSIGNED_BYTE
// data.pixels8.data()
//
// returns a RawImage containing the OpenGL texture ID, dimensions,
// and whether the texture represents a preview or the full image.
RawImage uploadTexture(const ImageData &data)
{
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (data.is16Bit)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, data.width, data.height,
                     0, GL_RGB, GL_UNSIGNED_SHORT, data.pixels16.data());
    }
    else
    {
        GLenum format = data.channels == 4 ? GL_RGBA : GL_RGB;
        GLint internalFormat = data.channels == 4 ? GL_RGBA8 : GL_RGB8;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, data.width, data.height,
                     0, format, GL_UNSIGNED_BYTE, data.pixels8.data());
    }

    return RawImage{(unsigned int)texId, data.width, data.height, data.kind};
}
