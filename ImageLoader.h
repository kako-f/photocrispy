#pragma once
#include <string>
#include <optional>
#include <vector>
#include <cstdint>

enum class ImageKind {
    Preview,
    Full
};

struct ImageData {
    std::vector<uint8_t> pixels8;
    std::vector<uint16_t> pixels16;
    int width = 0;
    int height = 0;
    int channels = 3;
    bool is16Bit = false;
    ImageKind kind = ImageKind::Full;
};

struct LoadResult {
    uint64_t generation = 0;
    ImageData image;
};

using RawImageData = ImageData;

struct RawImage
{
    unsigned int textureId = 0;
    int width = 0;
    int height = 0;
    ImageKind kind = ImageKind::Full;
};

std::optional<RawImageData> decodeRawImage(const std::string& path);
RawImage uploadTexture(const RawImageData& data);

