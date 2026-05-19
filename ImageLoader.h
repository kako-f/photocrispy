#pragma once
#include <string>
#include <optional>
#include <vector>

// Structure to hold raw image data
struct RawImageData {
    std::vector<uint16_t> pixels;
    int width, height;
};
struct RawImage
{
    unsigned int textureId;
    int width, height;
};

std::optional<RawImageData> decodeRawImage(const std::string& path);
RawImage uploadTexture(const RawImageData& data);

