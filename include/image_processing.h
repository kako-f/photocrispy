#pragma once
#include <vector>
#include <fmt/core.h>

namespace ImageProcessing
{
    // Applies a brightness adjustment to an 8-bit RGB interleaved image.
    // data: The image pixel data (modified in place).
    // width: Width of the image in pixels.
    // height: Height of the image in pixels.
    // brightness: An integer value typically between -255 and 255
    //             A value of 0 means no change. Positive values make it brighter, negative darker.
    void applyBrightness(std::vector<unsigned char>& data, int width, int height, int brightness);

    // void applyContrast(std::vector<unsigned char>& data, int width, int height, float contrast);
}