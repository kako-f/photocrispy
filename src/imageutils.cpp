#include <vector>
#include <numeric>
#include <algorithm>
#include "imageutils.h"
#include <fmt/core.h>

namespace ImageUtils
{

    ImageUtils::HistogramData calculateHistogram(const RawProcessor::RawImageInfo &imageInfo)
    {
        HistogramData data;

        if (!imageInfo.success || imageInfo.data.empty() || imageInfo.width == 0 || imageInfo.height == 0)
        {
            // Return empty on error
            return data;
        }

        data.maxIntensityValue = (1 << imageInfo.bits) - 1;
        int num_bins = data.maxIntensityValue + 1;

        data.red.resize(num_bins, 0.0f);
        data.green.resize(num_bins, 0.0f);
        data.blue.resize(num_bins, 0.0f);
        data.luminance.resize(num_bins, 0.0f);

        // Bytes per pixel
        size_t pixelStride = imageInfo.colors * (imageInfo.bits / 8);
        size_t dataSize = imageInfo.width * imageInfo.height * pixelStride;
        // Check if the data size matches what's expected

        if (imageInfo.data.size() < dataSize)
        {
            fmt::print("Error: Image data size mismatch during histogram calculation.\n");
            return data;
        }

        for (int y = 0; y < imageInfo.height; y++)
        {
            for (int x = 0; x < imageInfo.width; x++)
            {
                size_t pixelOffset = (y * imageInfo.width + x) * pixelStride;
                unsigned int r = 0, g = 0, b = 0;
                // Handle different bit depths and color formats
                if (imageInfo.bits == 8)
                {
                    // Assuming RGB or RGBA (if colors == 4)
                    if (imageInfo.colors >= 3)
                    {
                        r = imageInfo.data[pixelOffset + 0];
                        g = imageInfo.data[pixelOffset + 1];
                        b = imageInfo.data[pixelOffset + 2];
                    }
                    else if (imageInfo.colors == 1)
                    { // Grayscale
                        r = g = b = imageInfo.data[pixelOffset];
                    }
                }
                else if (imageInfo.bits == 16)
                {
                    // Data is stored as unsigned char, so 2 bytes per 16-bit channel
                    // Assuming little-endian for simplicity, adjust if needed
                    if (imageInfo.colors >= 3)
                    {
                        r = (imageInfo.data[pixelOffset + 1] << 8) | imageInfo.data[pixelOffset + 0];
                        g = (imageInfo.data[pixelOffset + 3] << 8) | imageInfo.data[pixelOffset + 2];
                        b = (imageInfo.data[pixelOffset + 5] << 8) | imageInfo.data[pixelOffset + 4];
                    }
                    else if (imageInfo.colors == 1)
                    { // Grayscale 16-bit
                        r = g = b = (imageInfo.data[pixelOffset + 1] << 8) | imageInfo.data[pixelOffset + 0];
                    }
                }
                else
                {
                    // Unsupported bit depth, return empty histogram
                    fmt::print("Unsupported bit depth for histogram: {} bits\n", imageInfo.bits);
                    return HistogramData();
                }

                // Increment histogram bins
                if (r < num_bins)
                    data.red[r]++;
                if (g < num_bins)
                    data.green[g]++;
                if (b < num_bins)
                    data.blue[b]++;

                // Calculate luminance for the luminance histogram
                // A common approximation: L = 0.299*R + 0.587*G + 0.114*B
                unsigned int luminance_val = static_cast<unsigned int>(
                    0.299f * r + 0.587f * g + 0.114f * b);
                if (luminance_val < num_bins)
                    data.luminance[luminance_val]++;
            }
        }
        // Normalize histograms
        float max_red = *std::max_element(data.red.begin(), data.red.end());
        float max_green = *std::max_element(data.green.begin(), data.green.end());
        float max_blue = *std::max_element(data.blue.begin(), data.blue.end());
        float max_luminance = *std::max_element(data.luminance.begin(), data.luminance.end());

        // Find the overall maximum value across all channels for consistent scaling
        float overall_max = std::max({max_red, max_green, max_blue, max_luminance});

        if (overall_max > 0)
        {
            for (float &val : data.red)
                val /= overall_max;
            for (float &val : data.green)
                val /= overall_max;
            for (float &val : data.blue)
                val /= overall_max;
            for (float &val : data.luminance)
                val /= overall_max;
        }

        return data;
    }
}
