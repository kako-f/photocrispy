#pragma once
#include <vector>
#include <array>
#include <string>
#include "raw_processing.h"

namespace ImageUtils
{
    struct HistogramData
    {
        std::vector<float> red;
        std::vector<float> green;
        std::vector<float> blue;
        std::vector<float> luminance; // Or grayscale, depending on needs
        int maxIntensityValue;      // The maximum possible intensity for the bit depth
    };
    HistogramData calculateHistogram(const RawProcessor::RawImageInfo &imageInfo);
} 