#pragma once
#include <vector>
#include <array>
#include <string>
#include "raw_processing.h"

namespace ImageUtils
{
    struct HistogramData
    {
        // Luminance can be changed to grayscale
        // to be implemented though
        // -
        // max intensity value depends of bit depth
        // (e.g., 255 for 8-bit, 65535 for 16-bit)
        std::vector<float> red;
        std::vector<float> green;
        std::vector<float> blue;
        std::vector<float> luminance;
        int maxIntensityValue = 0;
        int numBins = 0;
    };
    HistogramData calculateHistogram(const RawProcessor::RawImageInfo &imageInfo);
    void drawOverlayedHistogram(const HistogramData &data, float graph_width, float graph_height);
}