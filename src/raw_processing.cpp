#include "libraw.h"
#include "raw_processing.h"
#include <fmt/core.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace RawProcessor
{
    RawImageInfo RawProcessor::loadRaw(const std::string &raw_image_filepath)
    {
        LibRaw photoRawProcessor;
        RawImageInfo result;
        int ret;

        // Open RAW  photo file
        if (ret = photoRawProcessor.open_file(raw_image_filepath.c_str()) != LIBRAW_SUCCESS)
        {
            fmt::print("Error opening {}", raw_image_filepath.c_str());
            result.success = false;
        }

        // unpacking the data
        if (ret = photoRawProcessor.unpack() != LIBRAW_SUCCESS)
        {
            fmt::print("Error getting the data of {}", raw_image_filepath.c_str());
            photoRawProcessor.recycle();
            result.success = false;
        }

        //  processing pipeline, to modify
        if (ret = photoRawProcessor.dcraw_process() != LIBRAW_SUCCESS)
        {
            fmt::print("Cannot process {}", raw_image_filepath.c_str());
            photoRawProcessor.recycle();
            result.success = false;
        }

        // Process RAW data
        libraw_processed_image_t *image = photoRawProcessor.dcraw_make_mem_image();
        if (!image)
        {
            fmt::print("Failed to create processed image in memory {}", raw_image_filepath.c_str());
            photoRawProcessor.recycle();
            result.success = false;
        }
        else
        {
            result.success = true;
            result.is_raw_encoded = true;
            result.width = image->width;
            result.height = image->height;

            result.colors = image->colors;
            result.bits = image->bits;

            result.cam_make = photoRawProcessor.imgdata.idata.normalized_make;
            result.cam_model = photoRawProcessor.imgdata.idata.normalized_model;

            // Copy pixel data
            size_t size = image->width * image->height * image->colors;
            result.data = std::vector<unsigned char>(image->data, image->data + size);
            // clearing memory and recycling
            photoRawProcessor.dcraw_clear_mem(image);
            photoRawProcessor.recycle();
        }

        return result;
    }
    RawImageInfo RawProcessor::loadJpgPreview(const std::string &raw_image_filepath)
    {
        LibRaw photoRawProcessor;
        RawImageInfo result;
        int ret;

        // Open RAW  photo file
        if (ret = photoRawProcessor.open_file(raw_image_filepath.c_str()) != LIBRAW_SUCCESS)
        {
            fmt::print("Error opening {}", raw_image_filepath.c_str());
            result.success = false;
        }
        // unpack thumbnail
        if (ret = photoRawProcessor.unpack_thumb() != LIBRAW_SUCCESS)
        {
            fmt::print("Error getting thumbnail {}", raw_image_filepath.c_str());
            photoRawProcessor.recycle();
            result.success = false;
        }
        // if thumbnail.
        if (photoRawProcessor.imgdata.thumbnail.thumb)
        {
            int thumbWidth = photoRawProcessor.imgdata.thumbnail.twidth;
            int thumbHeight = photoRawProcessor.imgdata.thumbnail.theight;

            result.success = true;

            unsigned char *decoded_pixels = nullptr;
            int comp = 0; // Number of components (channels) when decoded by stb_image

            // if JPG
            if (photoRawProcessor.imgdata.thumbnail.tformat == LIBRAW_THUMBNAIL_JPEG)
            {
                fmt::print("JPG THUMBNAIL");

                result.is_jpeg_encoded = true;

                decoded_pixels = stbi_load_from_memory(
                    (const unsigned char *)photoRawProcessor.imgdata.thumbnail.thumb,
                    photoRawProcessor.imgdata.thumbnail.tlength,
                    &thumbWidth,
                    &thumbHeight,
                    &comp, result.colors); // 4 Channels - RGBA
                fmt::print("comp {}\n", comp);
            }

            result.width = thumbWidth;
            result.height = thumbHeight;
            result.colors = photoRawProcessor.imgdata.thumbnail.tcolors;
            result.cam_make = photoRawProcessor.imgdata.idata.normalized_make;
            result.cam_model = photoRawProcessor.imgdata.idata.normalized_model;
            result.jpgdata = decoded_pixels;
        }
        else
        {
            fmt::print("\n no data");
            photoRawProcessor.recycle();
            result.success = false;
        }
        // Always clean up LibRaw's internal buffers
        photoRawProcessor.recycle();
        return result;
    }

}