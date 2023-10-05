#pragma omp parallel for

#include <iostream>
#include <string>
#include <vector>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
OIIO_NAMESPACE_USING

#include "timer.h"


bool reverse_compare(double a, double b)
{
    return a > b;
}

void normalize_image(std::vector<float>& pixels)
{
    Timer timer("normalize_image");

    std::vector<float> buffer;
    buffer.assign(pixels.begin(), pixels.end());
    std::sort(buffer.begin(), buffer.end(), reverse_compare);
    float scale_factor = buffer[floor(0.01 * buffer.size())];
    for (int i = 0; i < pixels.size(); ++i)
    {
        pixels[i] = pixels[i] == 0 ? 0 : pixels[i] / scale_factor;
    }
}

void apply_gamma(std::vector<float>& pixels, float gamma)
{
    Timer timer("set_gamma");

    for (int i = 0; i < pixels.size(); ++i)
    {

        pixels[i] = pow(pixels[i], (1.0f / gamma));
    }
}

std::vector<float> read_image(const std::string& source_path, int& width, int& height, int& channels)
{
    Timer timer("read_image");

    auto in_file = OIIO::ImageInput::open(source_path);
    if (!in_file)
    {
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

std::vector<float> read_and_resize_image(const std::string& source_path, int& width, int& height, int& channels, int new_width)
{
    Timer timer("read_and_resize_image");

    auto in_file = ImageInput::open(source_path);
    if (!in_file)
    {
        std::cerr << "Source file is invalid or does not exist!" << std::endl;
        return {};
    }

    const ImageSpec& file_spec = in_file->spec();
    width = file_spec.width;
    height = file_spec.height;
    channels = file_spec.nchannels;
    std::cout << "Original size: " << width << "x" << height << ";" << std::endl;

    std::vector<float> pixels(width * height * channels);
    in_file->read_image(TypeDesc::FLOAT, pixels.data());
    in_file->close();

    // Resize logic
    int new_height = static_cast<int>(height * (static_cast<float>(new_width) / static_cast<float>(width)));
    
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
    resized_buf.get_pixels(resized_buf.roi(), TypeDesc::FLOAT, resized_pixels.data());
    
    // Normalizing
    normalize_image(resized_pixels); 

    return resized_pixels;
}

std::vector<float> scanline_image(const std::string& source_path, int& width, int& height, int& channels, int new_width)
{   
    Timer timer("scanline_image");

    auto in_file = OIIO::ImageInput::open(source_path);
    if (!in_file)
    {
        std::cerr << "Source file is invalid or does not exist!" << std::endl;
        return {};
    }
    const OIIO::ImageSpec& file_spec = in_file->spec();
    width = file_spec.width;
    height = file_spec.height;
    channels = file_spec.nchannels;
    std::cout << "Original size: " << width << "x" << height << ";" << std::endl;

    // calculate new height
    int new_height = static_cast<int>(float(height) / float(width) * new_width);
    std::vector<float> pixels(new_width * new_height * channels);
    std::vector<float> scanline(width * channels);
    
    if (file_spec.tile_width == 0) {
        // reshaping
        int lowres_pixel_index = 0;
        for (int y = 0; y < new_height; ++y) {
            // read highres scanline
            int highres_line_index = static_cast<int>(round(float(y) / float(new_height) * float(height)));
            in_file->read_scanline(highres_line_index, 0, OIIO::TypeDesc::FLOAT, &scanline[0]);
            for (int x = 0; x < new_width; ++x) {
                int highres_line_pixel_index = static_cast<int>(round(float(x) / float(new_width) * float(width)) * channels);
                for (int chnl = 0; chnl < channels; ++chnl) {
                    pixels[lowres_pixel_index + chnl] = scanline[highres_line_pixel_index + chnl];
                }
                lowres_pixel_index += channels;
            }
        }
    }
    else {
        std::cout << "Image is tiled! Cannot do anymore" << std::endl;
    }
    normalize_image(pixels); 
    width = new_width;
    height = new_height;
    return pixels;
}
    

std::vector<float> process_image(std::vector<float>& pixels, int width, int height, int channels, float gamma)
{
    apply_gamma(pixels, gamma);  
    return pixels;
}

bool write_image(const std::string& target_path, const std::vector<float>& pixels, int width, int height, int channels)
{
    Timer timer("write_image");

    auto out_file = OIIO::ImageOutput::create(target_path);
    if (!out_file)
    {
        std::cerr << "Cannot create output file!" << std::endl;
        return false;
    }
    
    OIIO::ImageSpec thumb_spec(width, height, channels, OIIO::TypeDesc::FLOAT);
    out_file->open(target_path, thumb_spec);
    out_file->write_image(OIIO::TypeDesc::FLOAT, pixels.data());
    out_file->close();

    return true;
}

int main()
{
    std::string source_path =  "examples/HDR_001.exr";
    std::string target_path = "examples/HDR_001.jpg";
    int NEW_WIDTH = 1024;
    int width, height, channels;
    // * READ IMAGE
    // std::vector<float> pixels = read_image(source_path, width, height, channels); // just read
    // std::vector<float> pixels = read_and_resize_image(source_path, width, height, channels, NEW_WIDTH); // read & resize
    std::vector<float> pixels = scanline_image(source_path, width, height, channels, NEW_WIDTH); // scanline read & resize > Fastest 
    
    if (pixels.empty())
    {
        std::cerr << "Failed to read image!" << std::endl;
        return 1;
    }
    
    float gamma = 2.2f;
    std::vector<float> processed_pixels = process_image(pixels, width, height, channels, gamma);
    if (!write_image(target_path, processed_pixels, width, height, channels)) {
        std::cerr << "Failed to write image!" << std::endl;
        return 1;
    }
    
    std::cout << " > Image written successfully to " << target_path << std::endl;
    return 0;
}


