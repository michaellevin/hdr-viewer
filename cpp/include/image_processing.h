#pragma once

#include <string>
#include <vector>

std::vector<float> scanline_image(
    const std::string& source_path, 
    int& width, 
    int& height, 
    int& channels, 
    int new_width
);

std::vector<float> process_image(
    std::vector<float>& pixels, 
    float gamma
);

bool write_image(
    const std::string& target_path, 
    const std::vector<float>& pixels, 
    int width, 
    int height, 
    int channels
);