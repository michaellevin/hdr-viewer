#define CL_HPP_TARGET_OPENCL_VERSION 300

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <omp.h>

#include <iostream>
#include <string>
#include <vector>

OIIO_NAMESPACE_USING
// #include <CL/cl2.hpp>
#include <CL/opencl.hpp>

#include "image_processing.h"
#include "timer.h"

ImageProcessor::ImageProcessor() {
    // Load OpenCL source code
    std::ifstream sourceFile("../../cpp/kernels/gamma_correction.cl");
    std::string sourceCode(std::istreambuf_iterator<char>(sourceFile),
                           (std::istreambuf_iterator<char>()));
    cl::Program::Sources source(1,
                                {sourceCode.c_str(), sourceCode.length() + 1});

    // Setup OpenCL environment
    context = cl::Context(CL_DEVICE_TYPE_GPU);
    program = cl::Program(context, source);

    // Build the OpenCL program and check for errors
    if (program.build({}) != CL_SUCCESS) {
        std::cerr << "OpenCL build error: "
                  << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(
                         context.getInfo<CL_CONTEXT_DEVICES>()[0])
                  << std::endl;
        exit(EXIT_FAILURE);
    }
}

void ImageProcessor::apply_gamma_correction(std::vector<float>& pixels,
                                            float inv_gamma) {
    // Prepare buffers
    cl::Buffer buffer_pixels(context, CL_MEM_READ_WRITE,
                             pixels.size() * sizeof(float));
    cl::CommandQueue queue(context);
    queue.enqueueWriteBuffer(buffer_pixels, CL_TRUE, 0,
                             pixels.size() * sizeof(float), pixels.data());

    // Set kernel arguments
    cl::Kernel kernel(program, "apply_gamma");
    kernel.setArg(0, buffer_pixels);
    kernel.setArg(1, static_cast<unsigned int>(pixels.size()));
    kernel.setArg(2, inv_gamma);

    // Execute kernel
    size_t global_work_size = pixels.size();
    queue.enqueueNDRangeKernel(kernel, cl::NullRange,
                               cl::NDRange(global_work_size), cl::NullRange);

    // Retrieve data
    queue.enqueueReadBuffer(buffer_pixels, CL_TRUE, 0,
                            pixels.size() * sizeof(float), pixels.data());
}

bool reverse_compare(double a, double b) { return a > b; }

void normalize_image(std::vector<float>& pixels) {
    Timer timer("normalize_image");

    std::vector<float> buffer;
    buffer.assign(pixels.begin(), pixels.end());
    std::sort(buffer.begin(), buffer.end(), reverse_compare);
    float scale_factor = buffer[floor(0.01 * buffer.size())];
    for (int i = 0; i < pixels.size(); ++i) {
        pixels[i] = pixels[i] == 0 ? 0 : pixels[i] / scale_factor;
    }
}

void apply_gamma(std::vector<float>& pixels, float gamma) {
    // Timer timer("set_gamma");
    float inv_gamma = 1.0f / gamma;
    for (int i = 0; i < pixels.size(); ++i) {
        pixels[i] = pow(pixels[i], inv_gamma);
    }
}

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

std::vector<float> scanline_image(const std::string& source_path, int& width,
                                  int& height, int& channels, int new_width) {
    Timer timer("scanline_image");

    auto in_file = OIIO::ImageInput::open(source_path);
    if (!in_file) {
        std::cerr << "Source file is invalid or does not exist!" << std::endl;
        return {};
    }
    const OIIO::ImageSpec& file_spec = in_file->spec();
    width = file_spec.width;
    height = file_spec.height;
    channels = file_spec.nchannels;
    std::cout << "Original size " << width << "x" << height << ";" << std::endl;

    // calculate new height
    int new_height = static_cast<int>(float(height) / float(width) * new_width);
    std::vector<float> pixels(new_width * new_height * channels);
    std::vector<float> scanline(width * channels);

    if (file_spec.tile_width == 0) {
        // reshaping
        int lowres_pixel_index = 0;
        for (int y = 0; y < new_height; ++y) {
            // read highres scanline
            int highres_line_index = static_cast<int>(
                round(float(y) / float(new_height) * float(height)));
            in_file->read_scanline(highres_line_index, 0, OIIO::TypeDesc::FLOAT,
                                   &scanline[0]);
            for (int x = 0; x < new_width; ++x) {
                int highres_line_pixel_index = static_cast<int>(
                    round(float(x) / float(new_width) * float(width)) *
                    channels);
                for (int chnl = 0; chnl < channels; ++chnl) {
                    pixels[lowres_pixel_index + chnl] =
                        scanline[highres_line_pixel_index + chnl];
                }
                lowres_pixel_index += channels;
            }
        }
    } else {
        std::cout << "Image is tiled! Cannot do anymore" << std::endl;
    }
    normalize_image(pixels);
    width = new_width;
    height = new_height;
    return pixels;
}

std::vector<float> process_image(std::vector<float>& pixels, float gamma) {
    Timer timer("apply gamma");
    // apply_gamma(pixels, gamma);
    float inv_gamma = 1.0f / gamma;
    apply_gamma_correction(pixels, inv_gamma);
    return pixels;
}

bool write_image(const std::string& target_path,
                 const std::vector<float>& pixels, int width, int height,
                 int channels) {
    Timer timer("write_image");

    auto out_file = OIIO::ImageOutput::create(target_path);
    if (!out_file) {
        std::cerr << "Cannot create output file!" << std::endl;
        return false;
    }

    OIIO::ImageSpec thumb_spec(width, height, channels, OIIO::TypeDesc::FLOAT);
    out_file->open(target_path, thumb_spec);
    out_file->write_image(OIIO::TypeDesc::FLOAT, pixels.data());
    out_file->close();

    return true;
}