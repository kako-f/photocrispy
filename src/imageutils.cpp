#include <vector>
#include <numeric>
#include <algorithm>
#include "imageutils.h"
#include "imgui.h"
#include <fmt/core.h>

namespace ImageUtils
{
    void drawOverlayedHistogram(const HistogramData &data, float graph_width, float graph_height)
    {
        if (data.red.empty())
        {
            ImGui::Text("No histogram data to display.");
            return;
        }

        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImVec2 p_min = ImGui::GetCursorScreenPos(); // Top-left of the drawing area
        ImVec2 p_max = ImVec2(p_min.x + graph_width, p_min.y + graph_height);

        // Advance cursor to make space for the plot
        // low level layout helper
        ImGui::Dummy(ImVec2(graph_width, graph_height));

        drawList->AddRectFilled(p_min, p_max, IM_COL32(30, 30, 30, 255)); // Dark Gray bg
        drawList->AddRect(p_min, p_max, IM_COL32(100, 100, 100, 255));    // border

        float hist_x_scale = graph_width / (data.numBins - 1); // Assuming data.numBins is now a member
        float hist_y_scale = graph_height;                     // Normalized values go from 0 to 1

        // Luminance
        if (!data.luminance.empty())
        {
            drawList->AddPolyline((const ImVec2 *)NULL, 0, IM_COL32(255, 255, 255, 255), false, 1.5f); // Start a new polyline, White
            for (int i = 0; i < data.luminance.size(); i++)
            {
                float x = p_min.x + i * hist_x_scale;
                float y = p_min.y + (graph_height - data.luminance[i] * hist_y_scale);
                drawList->PathLineTo(ImVec2(x, y));
            }
            drawList->PathStroke(IM_COL32(255, 255, 255, 255), false, 1.5f); // Stroke it white
        }
        // red
        if (!data.red.empty())
        {
            drawList->AddPolyline((const ImVec2 *)NULL, 0, IM_COL32(255, 0, 0, 255), false, 1.5f); // Start a new polyline, White
            for (int i = 0; i < data.red.size(); i++)
            {
                float x = p_min.x + i * hist_x_scale;
                float y = p_min.y + (graph_height - data.red[i] * hist_y_scale);
                drawList->PathLineTo(ImVec2(x, y));
            }
            drawList->PathStroke(IM_COL32(255, 0, 0, 255), false, 1.5f); // Stroke it red
        }
        // red
        if (!data.green.empty())
        {
            drawList->AddPolyline((const ImVec2 *)NULL, 0, IM_COL32(0, 255, 0, 255), false, 1.5f); // Start a new polyline, White
            for (int i = 0; i < data.green.size(); i++)
            {
                float x = p_min.x + i * hist_x_scale;
                float y = p_min.y + (graph_height - data.green[i] * hist_y_scale);
                drawList->PathLineTo(ImVec2(x, y));
            }
            drawList->PathStroke(IM_COL32(0, 255, 0, 255), false, 1.5f); // Stroke it green
        }
        // red
        if (!data.blue.empty())
        {
            drawList->AddPolyline((const ImVec2 *)NULL, 0, IM_COL32(0, 0, 255, 255), false, 1.5f); // Start a new polyline, White
            for (int i = 0; i < data.blue.size(); i++)
            {
                float x = p_min.x + i * hist_x_scale;
                float y = p_min.y + (graph_height - data.blue[i] * hist_y_scale);
                drawList->PathLineTo(ImVec2(x, y));
            }
            drawList->PathStroke(IM_COL32(0, 0, 255, 255), false, 1.5f); // Stroke it blue
        }
    }

    ImageUtils::HistogramData calculateHistogram(const RawProcessor::RawImageInfo &imageInfo)
    {
        HistogramData data;
        // Basic validation
        if (!imageInfo.success || imageInfo.data.empty() || imageInfo.width == 0 || imageInfo.height == 0)
        {
            // Return empty on error
            return HistogramData();
        }
        // Determining the maximum possible intensity value for a single color channel
        // based on the 'bits' per channel.
        // E.g., for 8 bits: (1 << 8) - 1 = 256 - 1 = 255 (intensities from 0 to 255)
        // For 16 bits: (1 << 16) - 1 = 65536 - 1 = 65535 (intensities from 0 to 65535)
        // --
        // Applying a left shift operation "<<". For example.
        // 1 << 8 -> 0b1 << 8 -> 0b100000000 = equals 256 in decimal
        data.maxIntensityValue = (1 << imageInfo.bits) - 1;
        // The number of bins is maxIntensityValue + 1 because we start from 0.
        // For example: 8-bit, values 0-255 need 256 bins.
        data.numBins = data.maxIntensityValue + 1;

        // Resizing each vector to hold 'numBins' elements, and start them in 0.0f (0 float)
        data.red.resize(data.numBins, 0.0f);
        data.green.resize(data.numBins, 0.0f);
        data.blue.resize(data.numBins, 0.0f);
        data.luminance.resize(data.numBins, 0.0f);

        // The total number of bytes occupied by a pixel in memory is called pixel stride
        // This determines how many bytes each pixel occupies in the 'data' vector.
        // imageInfo.colors: Number of channels (e.g., 3 for RGB, 4 for RGBA, 1 for Grayscale).
        // imageInfo.bits / 8: Bytes per channel (e.g., 8 bits = 1 byte, 16 bits = 2 bytes).
        size_t pixelStride = imageInfo.colors * (imageInfo.bits / 8);
        // Expected size of the image data
        size_t dataSize = imageInfo.width * imageInfo.height * pixelStride;

        // Check if the data size matches what's expected
        if (imageInfo.data.size() < dataSize)
        {
            fmt::print("Error: Image data size mismatch during histogram calculation. Expected: {} bytes, Got: {} bytes\n", dataSize, imageInfo.data.size());
            return HistogramData();
        }

        // Pixel iteration
        // Every pixel of the image data is visited. Nested loop
        for (int y = 0; y < imageInfo.height; y++)
        {
            for (int x = 0; x < imageInfo.width; x++)
            {
                // Calculate the starting byte offset for the current pixel.
                // (row_index * row_width + column_index) * bytes_per_pixel
                size_t pixelOffset = (y * imageInfo.width + x) * pixelStride;

                // Temporary variables to hold channel intensity
                unsigned int r = 0, g = 0, b = 0;

                // Extract Channel /(RGBA) data based on bith depth.
                if (imageInfo.bits == 8)
                {
                    // Assuming RGB or RGBA (if colors == 4)
                    if (imageInfo.colors >= 3)
                    {
                        r = imageInfo.data[pixelOffset + 0];
                        g = imageInfo.data[pixelOffset + 1];
                        b = imageInfo.data[pixelOffset + 2];
                    }
                    // Grayscale
                    else if (imageInfo.colors == 1)
                    {
                        r = g = b = imageInfo.data[pixelOffset];
                    }
                }
                else if (imageInfo.bits == 16)
                {
                    // TODO: Study.
                    // If 16 bits per channel, each intensity value is two bytes.
                    // We reconstruct the 16-bit value from two 8-bit unsigned chars.
                    // This assumes Little-Endian byte order (LSB first).
                    // Example: If 16-bit value is 0x1234, data[offset] = 0x34, data[offset+1] = 0x12
                    // (data[offset+1] << 8) shifts the most significant byte to its correct position.
                    // | data[offset] combines it with the least significant byte.
                    if (imageInfo.colors >= 3)
                    {
                        r = (imageInfo.data[pixelOffset + 1] << 8) | imageInfo.data[pixelOffset + 0];
                        g = (imageInfo.data[pixelOffset + 3] << 8) | imageInfo.data[pixelOffset + 2];
                        b = (imageInfo.data[pixelOffset + 5] << 8) | imageInfo.data[pixelOffset + 4];
                    }
                    // Grayscale 16-bit
                    else if (imageInfo.colors == 1)
                    {
                        r = g = b = (imageInfo.data[pixelOffset + 1] << 8) | imageInfo.data[pixelOffset + 0];
                    }
                }
                else
                {
                    // Unsupported bit depth, return empty histogram
                    fmt::print("Unsupported bit depth for histogram: {} bits\n", imageInfo.bits);
                    return HistogramData();
                }

                // Increment histogram bins with the channel data
                if (r < data.numBins)
                    data.red[r]++;
                if (g < data.numBins)
                    data.green[g]++;
                if (b < data.numBins)
                    data.blue[b]++;

                // Calculate and Increment Luminance Bin
                // Luminance (or perceived brightness) is calculated as a weighted average
                // of the RGB channels.
                // These specific weights (0.299, 0.587, 0.114) are standard for converting RGB to Y (luminance)
                // in color spaces like YCbCr (BT.601 standard).
                unsigned int luminance_val = static_cast<unsigned int>(
                    0.299f * r + 0.587f * g + 0.114f * b);
                // Ensure luminance value is within the valid bin range.
                if (luminance_val < data.numBins)
                    data.luminance[luminance_val]++;
            }
        }

        // Normalize histograms
        // Histograms are raw counts, meaning bins with more pixels will have higher numbers.
        // To display them effectively on a graph where the Y-axis range is, say, 0 to 1,
        // we need to normalize them. This means scaling all counts relative to the highest count found.
        float max_red = *std::max_element(data.red.begin(), data.red.end());
        float max_green = *std::max_element(data.green.begin(), data.green.end());
        float max_blue = *std::max_element(data.blue.begin(), data.blue.end());
        float max_luminance = *std::max_element(data.luminance.begin(), data.luminance.end());

        // Find the overall maximum value across all channels for consistent scaling
        // This is important for consistent scaling when plotting multiple histograms on the same graph.
        // If we normalized each individually, their peaks would all hit the top, making it hard
        // to compare their relative distributions.
        float overall_max = std::max({max_red, max_green, max_blue, max_luminance});
        
        // Divide every count in every histogram by the 'overall_max'.
        // After this, the highest count in *any* of the histograms will be 1.0,
        // and all other values will be between 0.0 and 1.0.
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
