#define CL_HPP_TARGET_OPENCL_VERSION 300
#define CL_HPP_ENABLE_EXCEPTIONS 1

#include "image_processing.h"

#include <CL/opencl.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "timer.h"

ImageProcessor::ImageProcessor() {
    // Load OpenCL source code
    std::vector<std::string> paths = {
        "C:/Cd/scripts/py/hdr-viewer/cpp/kernels/exposure_correction.cl",
        "C:/Cd/scripts/py/hdr-viewer/cpp/kernels/gamma_correction.cl"};

    cl::Program::Sources sources;

    // Loop through paths, load source codes
    for (const auto& path : paths) {
        std::ifstream sourceFile(path);
        if (!sourceFile.is_open()) {
            std::cerr << "Error opening OpenCL source file at " << path
                      << std::endl;
            exit(EXIT_FAILURE);
        }
        std::string sourceCode(std::istreambuf_iterator<char>(sourceFile),
                               (std::istreambuf_iterator<char>()));
        sources.push_back({sourceCode.c_str(), sourceCode.length() + 1});

        std::cout << "OpenCL Source Code from " << path << ":\n";
        std::cout << "------------------------------------\n";
        std::cout << sourceCode << "\n";
        std::cout << "------------------------------------\n";
    }

    context = cl::Context(CL_DEVICE_TYPE_GPU);
    program = cl::Program(context, sources);

    // Build and check for errors
    if (program.build({}) != CL_SUCCESS) {
        std::cerr << "OpenCL build error: "
                  << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(
                         context.getInfo<CL_CONTEXT_DEVICES>()[0])
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
    // Print device details
    for (const auto& device : devices) {
        std::string deviceName = device.getInfo<CL_DEVICE_NAME>();
        std::string deviceVendor = device.getInfo<CL_DEVICE_VENDOR>();
        cl_uint deviceComputeUnits =
            device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();

        std::cout << "Device Name: " << deviceName << "\n";
        std::cout << "Device Vendor: " << deviceVendor << "\n";
        std::cout << "Device Max Compute Units: " << deviceComputeUnits << "\n";
    }
}

// void ImageProcessor::apply_gamma_correction(std::vector<float>& pixels,
//                                             float inv_gamma) {
//     Timer timer("apply_gamma_correction");
//     cl::Buffer buffer_pixels(context, CL_MEM_READ_WRITE,
//                              pixels.size() * sizeof(float));
//     cl::CommandQueue queue(context);
//     queue.enqueueWriteBuffer(buffer_pixels, CL_TRUE, 0,
//                              pixels.size() * sizeof(float), pixels.data());
//     cl::Kernel kernel(program, "apply_gamma");
//     kernel.setArg(0, buffer_pixels);
//     kernel.setArg(1, static_cast<unsigned int>(pixels.size()));
//     kernel.setArg(2, inv_gamma);
//     size_t global_work_size = pixels.size();
//     queue.enqueueNDRangeKernel(kernel, cl::NullRange,
//                                cl::NDRange(global_work_size), cl::NullRange);
//     queue.enqueueReadBuffer(buffer_pixels, CL_TRUE, 0,
//                             pixels.size() * sizeof(float), pixels.data());
// }

/*
std::vector<float> ImageProcessor::apply_kernel(
    const std::vector<float>& pixels, const std::string& kernel_name,
    float parameter_value) const {
    Timer timer("apply_kernel: " + kernel_name);

    std::vector<float> modified_pixels = pixels;

    // Prepare buffers
    cl::Buffer buffer_pixels(context, CL_MEM_READ_WRITE,
                             modified_pixels.size() * sizeof(float));
    cl::CommandQueue queue(context);
    cl_int err = queue.enqueueWriteBuffer(
        buffer_pixels, CL_TRUE, 0, modified_pixels.size() * sizeof(float),
        modified_pixels.data());
    if (err != CL_SUCCESS) {
        std::cerr << "Error in enqueueWriteBuffer: " << err << "\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "Applying kernel: " << kernel_name << "\n";
    std::cout << "Parameter value: " << parameter_value << "\n";

    // Set kernel arguments
    cl::Kernel kernel(program, kernel_name.c_str());
    kernel.setArg(0, buffer_pixels);
    kernel.setArg(1, static_cast<unsigned int>(modified_pixels.size()));
    kernel.setArg(2, parameter_value);

    // Execute kernel
    size_t global_work_size = modified_pixels.size();
    err = queue.enqueueNDRangeKernel(
        kernel, cl::NullRange, cl::NDRange(global_work_size), cl::NullRange);
    if (err != CL_SUCCESS) {
        std::cerr << "Error in enqueueNDRangeKernel: " << err << "\n";
        exit(EXIT_FAILURE);
    }
    // Retrieve data
    err = queue.enqueueReadBuffer(buffer_pixels, CL_TRUE, 0,
                                  modified_pixels.size() * sizeof(float),
                                  modified_pixels.data());
    if (err != CL_SUCCESS) {
        std::cerr << "Error in enqueueReadBuffer: " << err << "\n";
        exit(EXIT_FAILURE);
    }
    // Return the modified pixel data
    return modified_pixels;
}

std::vector<float> ImageProcessor::apply_gamma_correction(
    const std::vector<float>& pixels, float inv_gamma) const {
    return apply_kernel(pixels, "apply_gamma", inv_gamma);
}

std::vector<float> ImageProcessor::apply_exposure_correction(
    const std::vector<float>& pixels, float exposure) const {
    return apply_kernel(pixels, "apply_exposure", exposure);
}
*/

std::vector<float> ImageProcessor::apply_gamma_correction(
    const std::vector<float>& pixels, float inv_gamma) const {
    Timer timer("apply_gamma_correction");

    std::vector<float> modified_pixels = pixels;

    cl::Buffer buffer_pixels(context, CL_MEM_READ_WRITE,
                             modified_pixels.size() * sizeof(float));
    cl::CommandQueue queue(context);
    queue.enqueueWriteBuffer(buffer_pixels, CL_TRUE, 0,
                             modified_pixels.size() * sizeof(float),
                             modified_pixels.data());

    cl::Kernel kernel(program, "apply_gamma");
    kernel.setArg(0, buffer_pixels);
    kernel.setArg(1, static_cast<unsigned int>(modified_pixels.size()));
    kernel.setArg(2, inv_gamma);

    size_t global_work_size = modified_pixels.size();
    queue.enqueueNDRangeKernel(kernel, cl::NullRange,
                               cl::NDRange(global_work_size), cl::NullRange);

    queue.enqueueReadBuffer(buffer_pixels, CL_TRUE, 0,
                            modified_pixels.size() * sizeof(float),
                            modified_pixels.data());

    return modified_pixels;
}

std::vector<float> ImageProcessor::apply_exposure_correction(
    const std::vector<float>& pixels, float exposure) const {
    Timer timer("apply_exposure_correction");

    std::vector<float> modified_pixels = pixels;

    cl::Buffer buffer_pixels(context, CL_MEM_READ_WRITE,
                             modified_pixels.size() * sizeof(float));
    cl::CommandQueue queue(context);
    queue.enqueueWriteBuffer(buffer_pixels, CL_TRUE, 0,
                             modified_pixels.size() * sizeof(float),
                             modified_pixels.data());

    // cl::Kernel kernel(program, "apply_exposure");
    cl_int err;
    cl::Kernel kernel;
    try {
        kernel = cl::Kernel(program, "apply_exposure", &err);
    } catch (cl::Error& e) {
        std::cerr << "Error: " << e.what() << ", " << e.err() << "\n";
    }
    kernel.setArg(0, buffer_pixels);
    kernel.setArg(1, static_cast<unsigned int>(modified_pixels.size()));
    kernel.setArg(2, exposure);

    size_t global_work_size = modified_pixels.size();
    err = queue.enqueueNDRangeKernel(
        kernel, cl::NullRange, cl::NDRange(global_work_size), cl::NullRange);
    if (err != CL_SUCCESS) {
        std::cerr << "Error in enqueueReadBuffer: " << err << "\n";
        exit(EXIT_FAILURE);
    }
    queue.enqueueReadBuffer(buffer_pixels, CL_TRUE, 0,
                            modified_pixels.size() * sizeof(float),
                            modified_pixels.data());

    return modified_pixels;
}