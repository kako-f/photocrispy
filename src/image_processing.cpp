#include "image_processing.h"
#include <algorithm>

namespace ImageProcessing
{
    void applyBrightness(std::vector<unsigned char> &data, int width, int height, int brightness)
    {
        // Ensure brightness is within a reasonable range (e.g., -255 to 255 for 8-bit)
        brightness = std::clamp(brightness, -255, 255);

        // Assuming RGB interleaved format (R G B R G B ...)
        // Each pixel has 3 channels (R, G, B)
        const int num_channels = 3;
        size_t total_pixels = width * height;

        for (size_t i = 0; i < total_pixels; ++i)
        {
            // Calculate the starting index for the current pixel's R, G, B components
            size_t pixel_start_index = i * num_channels;
            for (int c = 0; c < num_channels; ++c)
            {
                // Get the current channel value
                int value = data[pixel_start_index + c];

                // Apply brightness adjustment
                value += brightness;

                // Clamp the value to the valid 8-bit range (0 to 255)
                data[pixel_start_index + c] = static_cast<unsigned char>(std::clamp(value, 0, 255));
            }
        }
    }
}