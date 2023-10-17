#include "image_io.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "timer.h"

bool isHDRImage(const std::string& source_path) {
    // Retrieve the extension of the file from the file path.
    std::string extension =
        std::filesystem::path(source_path).extension().string();

    // Convert the extension to lowercase to ensure the comparison is
    // case-insensitive.
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Check if the extension corresponds to common HDR formats.
    return (extension == ".hdr" || extension == ".exr");
}

bool reverse_compare(double a, double b) { return a > b; }

void normalize_image(std::vector<float>& pixels) {
    Timer timer("normalize_image");

    std::vector<float> buffer;
    buffer.assign(pixels.begin(), pixels.end());
    std::sort(buffer.begin(), buffer.end(), reverse_compare);
    // float scale_factor = buffer[floor(0.01 * buffer.size())];
    size_t index =
        static_cast<size_t>(floor(0.01 * static_cast<double>(buffer.size())));
    float scale_factor = buffer[index];

    for (int i = 0; i < pixels.size(); ++i) {
        pixels[i] = pixels[i] == 0 ? 0 : pixels[i] / scale_factor;
    }
}

// void apply_gamma(std::vector<float>& pixels, float gamma) {
//     // Timer timer("set_gamma");
//     float inv_gamma = 1.0f / gamma;
//     for (int i = 0; i < pixels.size(); ++i) {
//         pixels[i] = pow(pixels[i], inv_gamma);
//     }
// }

DynamicRangeData find_dynamic_range(const std::vector<float>& pixels) {
    // Timer timer("find_dynamic_range");

    float max_pixel_value = FLT_MIN;  // Assume maximum pixel intensity in image
    float min_pixel_value = FLT_MAX;  // Assume minimum non-zero pixel intensity
                                      // in image

    // Loop through all pixels
    for (int i = 0; i < pixels.size(); i++) {
        // Find the maximum pixel value
        if (pixels[i] > max_pixel_value) {
            max_pixel_value = pixels[i];
        }

        // Find the minimum non-zero pixel value
        if (pixels[i] > 0.0f && pixels[i] < min_pixel_value) {
            min_pixel_value = pixels[i];
        }
    }

    // Calculate the dynamic range
    float dynamic_range = max_pixel_value / min_pixel_value;

    // Convert dynamic range to stops
    float stops = log2(dynamic_range);

    // Create and return the result struct
    DynamicRangeData result = {dynamic_range, stops};
    return result;
}

// READ IMAGE FUNCTIONS
std::vector<float> read_image(const std::string& source_path, int& width,
                              int& height, int& channels) {
    Timer timer("read_image");

    auto in_file = OIIO::ImageInput::open(source_path);
    if (!in_file) {
        std::cerr << "Source file is invalid or does not exist!" << std::endl;
        // Handle the error appropriately, e.g., by returning an empty vector
        return {};
    }

    const OIIO::ImageSpec& file_spec = in_file->spec();
    width = file_spec.width;
    height = file_spec.height;
    channels = file_spec.nchannels;

    std::vector<float> pixels(width * height * channels);
    in_file->read_image(OIIO::TypeDesc::FLOAT, pixels.data());

    in_file->close();

    return pixels;
}

std::vector<float> read_and_resize_image(const std::string& source_path,
                                         int& width, int& height, int& channels,
                                         int new_width) {
    Timer timer("read_and_resize_image");

    auto in_file = ImageInput::open(source_path);
    if (!in_file) {
        std::cerr << "Source file is invalid or does not exist!" << std::endl;
        return {};
    }

    const ImageSpec& file_spec = in_file->spec();
    width = file_spec.width;
    height = file_spec.height;
    channels = file_spec.nchannels;
    std::cout << "Original size: " << width << "x" << height << ";"
              << std::endl;

    std::vector<float> pixels(width * height * channels);
    in_file->read_image(TypeDesc::FLOAT, pixels.data());
    in_file->close();

    // Resize logic
    int new_height = static_cast<int>(
        height * (static_cast<float>(new_width) / static_cast<float>(width)));

    // Create an ImageBuf from the original pixel data
    ImageSpec orig_spec(width, height, channels, TypeDesc::FLOAT);
    ImageBuf original_buf(orig_spec, pixels.data());

    // Create a buffer for the resized image
    ImageSpec new_spec(new_width, new_height, channels, TypeDesc::FLOAT);
    ImageBuf resized_buf(new_spec);

    // Resize the image
    ImageBufAlgo::resize(resized_buf, original_buf);

    // Update width and height for the caller
    width = new_width;
    height = new_height;

    // Extract and return the pixel data from the resized buffer
    std::vector<float> resized_pixels(new_width * new_height * channels);
    resized_buf.get_pixels(resized_buf.roi(), TypeDesc::FLOAT,
                           resized_pixels.data());

    // Normalizing
    normalize_image(resized_pixels);

    return resized_pixels;
}

ImageData scanline_image(const std::string& source_path, int new_width) {
    Timer timer("scanline_image");

    // [01]. Reading file
    auto in_file = OIIO::ImageInput::open(source_path);
    if (!in_file) {
        std::cerr << "Source file is invalid or does not exist!" << std::endl;
        return {};
    }
    const OIIO::ImageSpec& file_spec = in_file->spec();
    int width = file_spec.width;
    int height = file_spec.height;
    int nchannels = file_spec.nchannels;
    OIIO::TypeDesc image_format = file_spec.format;
    std::cout << source_path << "\nSize " << width << "x" << height
              << " / Num channels: " << nchannels
              << " / Image format: " << image_format << std::endl;

    const std::vector<std::string>& channel_names = file_spec.channelnames;
    bool hasAlpha = (std::find(channel_names.begin(), channel_names.end(),
                               "A") != channel_names.end()) ||
                    (std::find(channel_names.begin(), channel_names.end(),
                               "Alpha") != channel_names.end());

    bool nonWhiteAlphaFound = false;  // Flag to identify if we've encountered
                                      // any non-white alpha value.
    int alphaChannelIndex =
        hasAlpha ? (nchannels - 1)
                 : -1;  // Generally, the alpha channel is the last one.

    // if nchannels in {3,4} => output_channels = nchannels
    // nchannels could be 2: Y,A (luminance + alpha). => output_channels = 4
    int output_nchannels = (nchannels == 3 || nchannels == 4) ? nchannels : 4;

    // [02] Preparing arrays of pixels and scanline, calculate new height
    int new_height = static_cast<int>(float(height) / float(width) * new_width);
    std::vector<float> pixels(new_width * new_height * output_nchannels);
    std::vector<float> scanline(width * nchannels);

    // [03] Reshaping pixels
    int lowres_pixel_index = 0;
    float alphaValue = 0.0f;
    for (int y = 0; y < new_height; ++y) {
        // read highres scanline
        int highres_line_index = static_cast<int>(
            round(float(y) / float(new_height) * float(height)));
        in_file->read_scanline(highres_line_index, 0, OIIO::TypeDesc::FLOAT,
                               &scanline[0]);
        for (int x = 0; x < new_width; ++x) {
            int highres_line_pixel_index = static_cast<int>(
                round(float(x) / float(new_width) * float(width)) * nchannels);

            if (nchannels == 2) {  // specific case for 2-channel images
                // Copy the Y (luminance) and A (alpha) channels.
                for (int chnl = 0; chnl < 3; ++chnl) {
                    pixels[lowres_pixel_index + chnl] =
                        scanline[highres_line_pixel_index];
                }
                alphaValue = scanline[highres_line_pixel_index +
                                      alphaChannelIndex];  // A
                pixels[lowres_pixel_index + 3] = alphaValue;
                if (hasAlpha && !nonWhiteAlphaFound) {
                    if (alphaValue < 1.0f) {
                        nonWhiteAlphaFound = true;  // If any alpha value is not
                                                    // white, set the flag true.
                    }
                }
                lowres_pixel_index += 4;  // proceed by 2 channels
            } else {
                // Normal scenario: copy all channels.
                for (int chnl = 0; chnl < output_nchannels; ++chnl) {
                    pixels[lowres_pixel_index + chnl] =
                        scanline[highres_line_pixel_index + chnl];
                    if (hasAlpha && !nonWhiteAlphaFound) {
                        if (chnl == alphaChannelIndex) {
                            alphaValue =
                                scanline[highres_line_pixel_index + chnl];
                            if (alphaValue < 1.0f) {
                                nonWhiteAlphaFound =
                                    true;  // If any alpha value is not white,
                                           // set the flag true.
                            }
                        }
                    }
                }
                lowres_pixel_index += output_nchannels;
            }
        }
    }

    // [04] Get rid of Alpha if it's not needed
    bool outputHasAlpha = hasAlpha && nonWhiteAlphaFound;
    if (hasAlpha && !nonWhiteAlphaFound) {
        std::vector<float> compact_pixels;
        // Reserves space for the new pixel array without the alpha channel.
        // This helps in avoiding multiple reallocations.
        compact_pixels.reserve((pixels.size() * 3) / 4);

        for (int i = 0; i < pixels.size(); i++) {
            if ((i + 1) % 4 != 0) {
                compact_pixels.push_back(pixels[i]);
            }
        }

        pixels.swap(compact_pixels);
        compact_pixels.clear();
        output_nchannels -= 1;
    }

    // [05] Create ImageData struct
    ImageData result;
    result.pixels =
        std::move(pixels);  // Move the pixel data into the result structure
    result.original_width = width;
    result.original_height = height;
    result.num_original_channels = nchannels;
    result.resized_width = new_width;
    result.resized_height = new_height;
    result.num_output_channels = output_nchannels;
    result.original_has_alpha = hasAlpha;
    result.output_has_alpha = outputHasAlpha;
    // [05i] Normalize image if it's HDR/EXR
    if (isHDRImage(source_path)) {
        // It's important that 'dynamicRangeData' is created with 'new', as it
        // is held by a unique_ptr
        result.dynamic_range_data = std::make_unique<DynamicRangeData>(
            find_dynamic_range(result.pixels));
        normalize_image(result.pixels);
    } else {
        result.dynamic_range_data = nullptr;  // No dynamic range data
    }
    std::cout << "New size: " << new_width << "x" << new_height << ";"
              << " output channels: " << result.num_output_channels
              << std::endl;
    return result;
}

// PROCESS IMAGE (REDUNDANT)
// std::vector<float> process_image(std::vector<float>& pixels, float gamma) {
//     Timer timer("apply gamma");
//     apply_gamma(pixels, gamma);
//     return pixels;
// }

// WRTIE IMAGE
bool write_image(const std::string& target_path,
                 const std::vector<float>& pixels, int width, int height,
                 int channels) {
    Timer timer("write_image");

    // Print the information to the console.
    // int num_pixels = pixels.size() / channels;
    // std::cout << "[WRITE] Number of pixels: " << num_pixels << std::endl;
    // std::cout << "[WRITE] Width: " << width << std::endl;
    // std::cout << "[WRITE] Height: " << height << std::endl;
    // std::cout << "[WRITE] Channels: " << channels << std::endl;

    auto out_file = OIIO::ImageOutput::create(target_path);
    if (!out_file) {
        std::cerr << "Cannot create output file" << std::endl;
        return false;
    }

    OIIO::ImageSpec thumb_spec(width, height, channels, OIIO::TypeDesc::FLOAT);
    out_file->open(target_path, thumb_spec);
    out_file->write_image(OIIO::TypeDesc::FLOAT, pixels.data());
    out_file->close();

    return true;
}