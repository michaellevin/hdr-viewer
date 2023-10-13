#pragma once
#include <iostream>
#include <string>
#include <vector>

struct DynamicRangeData {
    float dynamic_range;
    float stops;
};

struct ImageData {
    std::vector<float> pixels;
    DynamicRangeData dynamic_range_data;
};

DynamicRangeData find_dynamic_range(const std::vector<float>& pixels);

ImageData scanline_image(const std::string& source_path, int& width,
                         int& height, int& channels, int new_width);

std::vector<float> process_image(std::vector<float>& pixels, float gamma);

bool write_image(const std::string& target_path,
                 const std::vector<float>& pixels, int width, int height,
                 int channels);
