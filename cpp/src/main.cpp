#include <iostream>
#include <string>
#include <vector>

#include "image_processing.h"

int main() {
    std::string source_path = "examples/HDR_001.exr";
    std::string target_path = "examples/HDR_001.jpg";
    int NEW_WIDTH = 1024;
    int width, height, channels;
    // * READ IMAGE
    // std::vector<float> pixels = read_image(source_path, width, height,
    // channels); // just read std::vector<float> pixels =
    // read_and_resize_image(source_path, width, height, channels, NEW_WIDTH);
    // // read & resize
    std::vector<float> pixels =
        scanline_image(source_path, width, height, channels,
                       NEW_WIDTH);  // scanline read & resize > Fastest

    if (pixels.empty()) {
        std::cerr << "Failed to read image" << std::endl;
        return 1;
    }

    float gamma = 2.2f;
    std::vector<float> processed_pixels = process_image(pixels, gamma);
    if (!write_image(target_path, processed_pixels, width, height, channels)) {
        std::cerr << "Failed to write image" << std::endl;
        return 1;
    }

    std::cout << " > Image written successfully to " << target_path
              << std::endl;
    return 0;
}