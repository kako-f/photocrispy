#pragma once
#include <string>
#include <vector>


namespace RawProcessor
{
    struct RawImageInfo
    {
        bool success;
        int width = 0;
        int height = 0;

        int colors = 0;
        int bits = 0;
        std::string cam_make;
        std::string cam_model;

        std::vector<unsigned char> data; 

    };

    RawImageInfo loadRaw(const std::string &raw_image_filepath);
}