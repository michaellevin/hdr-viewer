#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct DynamicRangeData {
    float dynamic_range;
    float stops;
};

struct ImageData {
    std::vector<float> pixels;
    int original_width;
    int original_height;
    int num_original_channels;
    int resized_width;
    int resized_height;
    int num_output_channels;
    bool original_has_alpha;
    bool output_has_alpha;
    std::unique_ptr<DynamicRangeData> dynamic_range_data;
    // Utility function to check if dynamic range data exists
    bool hasDynamicRangeData() const { return dynamic_range_data != nullptr; }
};

DynamicRangeData find_dynamic_range(const std::vector<float>& pixels);

ImageData scanline_image(const std::string& source_path, int new_width);

// std::vector<float> process_image(std::vector<float>& pixels, float gamma);

bool write_image(const std::string& target_path,
                 const std::vector<float>& pixels, int width, int height,
                 int channels);
