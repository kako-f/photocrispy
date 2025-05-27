#include "libraw.h"
#include "raw_processing.h"
#include <fmt/core.h>

namespace RawProcessor
{

    RawImageInfo RawProcessor::loadRaw(const std::string &raw_image_filepath)
    {
        LibRaw photoRawProcessor;
        RawImageInfo result;

        // opening raw photo file
        int ret = photoRawProcessor.open_file(raw_image_filepath.c_str());
        if (ret != LIBRAW_SUCCESS)
        {
            return result;
        }
        // unpacking the data
        ret = photoRawProcessor.unpack();
        if (ret != LIBRAW_SUCCESS)
        {
            return result;
        }
        // reads (or unpacks) the default (largest) image preview (thumbnail)
        // int thumb = photoRawProcessor.unpack_thumb();
        // 
        ret = photoRawProcessor.dcraw_process();
        if (ret != LIBRAW_SUCCESS)
        {
            return result;
        }

        libraw_processed_image_t *image = photoRawProcessor.dcraw_make_mem_image();
        if (!image)
            return result;

        result.success = true;
        result.width = image->width;
        result.height = image->height;

        result.colors = image->colors;
        result.bits = image->bits;

        
        result.cam_make = photoRawProcessor.imgdata.idata.normalized_make;
        result.cam_model = photoRawProcessor.imgdata.idata.normalized_model;

        // Copy pixel data
        size_t size = image->width * image->height * image->colors;
        result.data = std::vector<unsigned char>(image->data, image->data + size);

        photoRawProcessor.dcraw_clear_mem(image);

        return result;
    }
}