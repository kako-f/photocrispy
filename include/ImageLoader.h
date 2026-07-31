#pragma once
#include <string>
#include <optional>
#include <vector>
#include <cstdint>

// Preview or full image. To make notice to the UI of what's loading
enum class ImageKind
{
    Preview,
    Full
};

// general struct for loading image data
// Works with jpg preview (8bit usually) and full 16bit raw data
// pixels8 is for lightweight previews. (uint8_t)
// pixels16 is for full-quality RAW output. (uint16_t)
// is16Bit tells uploadTexture() whether to upload with GL_UNSIGNED_BYTE or GL_UNSIGNED_SHORT.
// kind tells the app whether it is currently showing the quick preview or the final image.
struct ImageData
{
    std::vector<uint8_t> pixels8;
    std::vector<uint16_t> pixels16;
    int width = 0;
    int height = 0;
    int channels = 3;
    bool is16Bit = false;
    ImageKind kind = ImageKind::Full;
};
// If the user opens file A, then quickly opens file B, the app can discard late results from file A
// by checking the generation. That prevents stale preview/full results from replacing the newer image.
struct LoadResult
{
    uint64_t generation = 0;
    ImageData image;
};

struct RawImage
{
    unsigned int textureId = 0;
    int width = 0;
    int height = 0;
    ImageKind kind = ImageKind::Full;
};
// Decode and Extract - Exctrat Run first, then full decode of raw
std::optional<ImageData> decodeFullRawImage(const std::string &path);
std::optional<ImageData> extractEmbeddedPreviewImage(const std::string &path);
RawImage uploadTexture(const ImageData &data);
