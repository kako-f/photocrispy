#include "raw_processing.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "libraw.h"

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
            fmt::print(stderr, "Error opening {}: {}\n", raw_image_filepath, libraw_strerror(ret));
            return result;
        }
        // RAW Processing params.
        // to modify further.
        photoRawProcessor.imgdata.params.no_auto_bright = 1; // No auto brightness
        photoRawProcessor.imgdata.params.user_qual = 0;      // Fastest demosaic (linear)
        photoRawProcessor.imgdata.params.use_camera_wb = 1;       // No gamma correction
        photoRawProcessor.imgdata.params.use_camera_matrix = 0; // color profile

        // unpacking the data
        if (ret = photoRawProcessor.unpack() != LIBRAW_SUCCESS)
        {
            fmt::print(stderr, "Error getting the data of {}: {}\n", raw_image_filepath, libraw_strerror(ret));

            return result;
        }

        //  processing pipeline, to modify
        if (ret = photoRawProcessor.dcraw_process() != LIBRAW_SUCCESS)
        {
            fmt::print(stderr, "Cannot process {}: {}\n", raw_image_filepath, libraw_strerror(ret));

            return result;
        }

        // Process RAW data and get memory image
        libraw_processed_image_t *image = photoRawProcessor.dcraw_make_mem_image();
        if (!image)
        {
            fmt::print(stderr, "Failed to create processed image in memory for {}. Possible memory exhaustion or prior error.\n", raw_image_filepath);
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
            result.lens_model = photoRawProcessor.imgdata.lens.Lens;
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