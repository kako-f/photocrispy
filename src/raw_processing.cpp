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
        // --- LibRaw Processing Params - Linear High-Bit-Depth Output ---
        // to modify further. 1 enabled 0 disabled -- unless otherwise
        photoRawProcessor.imgdata.params.no_auto_bright = 1;    // Keep manual control over brightness
        photoRawProcessor.imgdata.params.user_qual = 0;         // Fastest demosaic, often fine for speed, but quality might vary.
                                                                // Consider `1` for VNG, `2` for AHD if quality is paramount.
        photoRawProcessor.imgdata.params.use_camera_wb = 0;     // Use camera's white balance (recommended)
        photoRawProcessor.imgdata.params.use_camera_matrix = 1; // Use camera's color profile (CRITICAL for correct colors)

        // --- Testing: Set output format to 16-bit linear ---
        photoRawProcessor.imgdata.params.output_bps = 16;  // Output 16 bits per sample
/*         photoRawProcessor.imgdata.params.gamm[0] = 1.0;    // Linear gamma (no gamma curve)
        photoRawProcessor.imgdata.params.gamm[1] = 1.0;    // Linear gamma (no gamma curve) */
        photoRawProcessor.imgdata.params.output_color = 1; // Output in sRGB color space (linear primaries)
                                                           // 0 for raw camera space, 1 for sRGB, 2 for Adobe, 3 for WideGamut, 4 for ProPhoto

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

            // size_t size = image->width * image->height * image->colors;
            
            // Copy pixel data          
            // For 16-bit data, size is width * height * colors * (bits / 8)
            size_t bytes_per_pixel = image->colors * (image->bits / 8);
            size_t total_bytes = image->width * image->height * bytes_per_pixel;

            // result.data = std::vector<unsigned char>(image->data, image->data + size);
            
            // Ensure result.data can hold 16-bit data (unsigned char is fine, but you'll treat it as unsigned short)
            // If you want to store as std::vector<unsigned short>, change RawImageInfo.data type.
            // For now, let's assume you'll cast it to unsigned short when passing to OpenGL.
            result.data.assign(image->data, image->data + total_bytes);
            fmt::print("{} bits\n" ,photoRawProcessor.imgdata.params.output_bps);
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