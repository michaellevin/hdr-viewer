#include <cmath>  // for std::round
#include <filesystem>
#include <iomanip>  // Include for std::setprecision
#include <iostream>
#include <sstream>  // Required for std::ostringstream
#include <string>
#include <vector>

#include "image_io.h"
#include "image_processing.h"

namespace fs = std::filesystem;
int NEW_WIDTH = 1024;
ImageProcessor imageProcessor;

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
    // Directory containing the image files
    std::string directory_path = "examples";

    // Check if the directory exists
    if (!fs::exists(directory_path) || !fs::is_directory(directory_path)) {
        std::cerr << "Directory does not exist: " << directory_path
                  << std::endl;
        return -1;
    }

    // Iterate over the files in the directory
    for (const auto& entry : fs::directory_iterator(directory_path)) {
        auto path = entry.path();
        std::string filename = path.filename().string();

        // Skip files with "_CORR" in their name
        if (filename.find("_CORR") != std::string::npos) {
            continue;
        }

        // Skip directories
        if (fs::is_directory(path)) {
            continue;
        }

        // Reading
        ImageData image_data = scanline_image(path.string(), NEW_WIDTH);
        if (image_data.pixels.empty()) {
            std::cerr << "Failed to load image: " << path.string() << std::endl;
            continue;  // Skip to the next file
        }
        if (image_data.hasDynamicRangeData()) {
            DynamicRangeData* hdr_info = image_data.dynamic_range_data.get();
            print_info(hdr_info->dynamic_range, hdr_info->stops);
        }

        // * PROCESS IMAGE
        float gamma = 2.2f;
        float inv_gamma = 1.0f / gamma;
        float exposure = 0.2f;
        std::vector<float> processed_pixels =
            imageProcessor.apply_exposure_gamma_correction(image_data.pixels,
                                                           exposure, inv_gamma);

        // * WRITE IMAGE
        std::string new_filename = path.stem().string() + "_CORR.png";
        std::string target_path =
            path.parent_path().string() + "/" + new_filename;
        write_image(target_path, image_data.pixels, image_data.resized_width,
                    image_data.resized_height, image_data.num_output_channels);
        std::cout << " > Image written successfully to " << target_path
                  << "\n-----------------" << std::endl;
    }
    return 0;
}