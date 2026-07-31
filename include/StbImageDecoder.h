#pragma once

#include "ImageLoader.h"

#include <cstddef>
#include <cstdint>
#include <optional>

std::optional<ImageData> decodeJpegMemoryToRgb(const uint8_t* data, size_t size, ImageKind kind);
