#include "StbImageDecoder.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

std::optional<ImageData> decodeJpegMemoryToRgb(const uint8_t* data, size_t size, ImageKind kind)
{
    if (!data || size == 0)
        return std::nullopt;

    int width = 0;
    int height = 0;
    int sourceChannels = 0;
    constexpr int requestedChannels = 3;

    unsigned char* decoded = stbi_load_from_memory(
        data,
        static_cast<int>(size),
        &width,
        &height,
        &sourceChannels,
        requestedChannels);

    if (!decoded)
        return std::nullopt;

    ImageData image;
    image.width = width;
    image.height = height;
    image.channels = requestedChannels;
    image.is16Bit = false;
    image.kind = kind;
    image.pixels8.assign(decoded, decoded + (width * height * requestedChannels));

    stbi_image_free(decoded);
    return image;
}
