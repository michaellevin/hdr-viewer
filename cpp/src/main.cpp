#include <cmath>    // for std::round
#include <iomanip>  // Include for std::setprecision
#include <iostream>
#include <sstream>  // Required for std::ostringstream
#include <string>
#include <vector>

#include "image_processing.h"

std::string format_dynamic_range(float dynamic_range) {
    std::ostringstream out;
    if (dynamic_range >= 1e6) {
        out << std::fixed << std::setprecision(2) << (dynamic_range / 1e6)
            << "M";
    } else if (dynamic_range >= 1e3) {
        out << std::fixed << std::setprecision(2) << (dynamic_range / 1e3)
            << "K";
    } else {
        out << std::fixed << std::setprecision(2) << dynamic_range;
    }
    return out.str();
}

std::string evaluate_image_quality(float dynamic_range, float stops) {
    // Evaluate the image quality based on dynamic range and stops together

    // Define some threshold values (customizable based on your needs)
    constexpr float HIGH_DYNAMIC_RANGE = 1e5;
    constexpr float HIGH_STOPS = 15;

    if (dynamic_range > HIGH_DYNAMIC_RANGE && stops > HIGH_STOPS) {
        return "Very High";
    } else if (dynamic_range > (HIGH_DYNAMIC_RANGE / 10) &&
               stops > (HIGH_STOPS - 5)) {
        return "High";
    } else {
        return "Standard";
    }
}

void print_info(float dynamic_range, float stops) {
    std::string quality = evaluate_image_quality(dynamic_range, stops);

    std::cout << "Dynamic Range: " << format_dynamic_range(dynamic_range)
              << " (" << quality << ")\n"
              << "Stops: " << std::fixed << std::setprecision(2) << stops
              << " (" << quality << ")\n";
}

int main() {
    std::string source_path = "examples/HDR_001.exr";
    std::string target_path = "examples/HDR_001.jpg";
    ImageProcessor imageProcessor;
    int NEW_WIDTH = 1024;
    int width, height, channels;
    // * READ IMAGE
    // std::vector<float> pixels = read_image(source_path, width, height,
    // channels); // just read std::vector<float> pixels =
    // read_and_resize_image(source_path, width, height, channels, NEW_WIDTH);
    // // read & resize
    ImageData image_data =
        scanline_image(source_path, width, height, channels,
                       NEW_WIDTH);  // scanline read & resize > Fastest
    std::vector<float> pixels = image_data.pixels;

    if (pixels.empty()) {
        std::cerr << "Failed to read image" << std::endl;
        return 1;
    }

    DynamicRangeData hdr_info = image_data.dynamic_range_data;
    print_info(hdr_info.dynamic_range, hdr_info.stops);

    float gamma = 2.5f;
    // std::vector<float> processed_pixels = process_image(pixels, gamma);
    // imageProcessor.apply_gamma_correction(pixels, 1.0f / gamma);
    std::vector<float> processed_pixels =
        imageProcessor.apply_gamma_correction(pixels, 1.0f / gamma);
    if (!write_image(target_path, processed_pixels, width, height, channels)) {
        std::cerr << "Failed to write image" << std::endl;
        return 1;
    }

    std::cout << " > Image written successfully to " << target_path
              << std::endl;
    return 0;
}