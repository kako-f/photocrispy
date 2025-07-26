#pragma once
#include <string>
#include <vector>
#include <fmt/core.h>

namespace RawProcessor
{
    struct RawImageInfo
    {

        bool success = false;

        bool is_jpeg_encoded = false;
        bool is_raw_encoded = false;

        int width = 0;
        int height = 0;

        int colors = 0;
        int bits = 0;

        std::string cam_make;
        std::string cam_model;
        std::string lens_model;
        std::string lens_focal;
        std::string lens_aperture;

        std::vector<unsigned char> data;
        //std::vector<unsigned short> data;
        unsigned char *jpgdata;
    };

    RawImageInfo loadRaw(const std::string &raw_image_filepath);
    RawImageInfo loadJpgPreview(const std::string &raw_image_filepath);
}