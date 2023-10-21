#include "image_processing.h"

#include <CL/opencl.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "timer.h"

/* Version A: Open several kernel files from the folder */
// ImageProcessor::ImageProcessor() {
//     try {
//         // Set GPU Context
//         context = cl::Context(CL_DEVICE_TYPE_GPU);
//         std::vector<cl::Device> devices =
//         context.getInfo<CL_CONTEXT_DEVICES>();
//         // Print device details
//         for (const auto& device : devices) {
//             std::string deviceName = device.getInfo<CL_DEVICE_NAME>();
//             std::string deviceVendor = device.getInfo<CL_DEVICE_VENDOR>();
//             cl_uint deviceComputeUnits =
//                 device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();

//             std::cout << "Device Name: " << deviceName << "\n";
//             std::cout << "Device Vendor: " << deviceVendor << "\n";
//             std::cout << "Device Max Compute Units: " << deviceComputeUnits
//                       << "\n";
//         }

//         // Load OpenCL source code
//         std::vector<std::string> paths = {
//             // "../../cpp/kernels/apply_gamma.cl",
//             // "../../cpp/kernels/apply_exposure.cl",
//             "../../cpp/kernels/apply_exposure_gamma.cl"};
//         std::filesystem::path currentPath = std::filesystem::current_path();
//         std::cout << "Current working directory: " << currentPath <<
//         std::endl; cl::Program::Sources sources; for (const auto& path :
//         paths) {
//             std::ifstream sourceFile(path);
//             if (!sourceFile.is_open()) {
//                 // std::cerr << "Error opening OpenCL source file at " <<
//                 path
//                 //           << std::endl;
//                 // exit(EXIT_FAILURE);
//                 throw std::runtime_error(
//                     "Error opening OpenCL source file at " + path);
//             }
//             std::string
//             sourceCode(std::istreambuf_iterator<char>(sourceFile),
//                                    (std::istreambuf_iterator<char>()));

//             // std::cout << "OpenCL Source Code from " << path << ":\n";
//             // std::cout << "------------------------------------\n";
//             // std::cout << sourceCode << "\n";
//             // std::cout << "------------------------------------\n";

//             cl::Program::Sources source;
//             source.push_back({sourceCode.c_str(), sourceCode.length() + 1});

//             cl::Program program(context, source);
//             if (program.build(devices) != CL_SUCCESS) {
//                 std::cerr << "OpenCL build error: "
//                           << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(
//                                  devices[0])
//                           << std::endl;
//                 // exit(EXIT_FAILURE);
//                 throw std::runtime_error(
//                     "OpenCL build error: " +
//                     program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]));
//             }
//             std::filesystem::path fs_path(path);
//             std::string key = fs_path.stem().string();
//             programs[key] = program;
//         }
//     } catch (const cl::Error& e) {
//         // Handle OpenCL exceptions
//         std::cerr << "Exception in ImageProcessor: " << e.what() << " : "
//                   << e.err() << std::endl;
//         throw;  // rethrow to propagate the error
//     } catch (const std::exception& e) {
//         // Handle standard exceptions
//         std::cerr << "Exception in ImageProcessor: " << e.what() <<
//         std::endl; throw;  // rethrow to propagate the error
//     } catch (...) {
//         // Handle all other types of exceptions
//         std::cerr << "An unknown exception occurred in ImageProcessor"
//                   << std::endl;
//         throw;  // rethrow to propagate the error
//     }
// }

// Version B: OpenCL kernel file in-line
ImageProcessor::ImageProcessor() {
    try {
        context = cl::Context(CL_DEVICE_TYPE_GPU);
        std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
        // Print device details
        for (const auto& device : devices) {
            std::string deviceName = device.getInfo<CL_DEVICE_NAME>();
            std::string deviceVendor = device.getInfo<CL_DEVICE_VENDOR>();
            cl_uint deviceComputeUnits =
                device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();

            std::cout << "Device Name: " << deviceName << "\n";
            std::cout << "Device Vendor: " << deviceVendor << "\n";
            std::cout << "Device Max Compute Units: " << deviceComputeUnits
                      << "\n";
        }

        // Define your OpenCL kernel code as a string.
        // This is a raw string literal encompassing multiple lines.
        std::string kernelCode = R"(
            __kernel void apply_exposure_gamma(
                __global float* pixels, 
                const unsigned int count,
                __global float* params, 
                const unsigned int param_count) 
            {
                if(param_count != 2) {
                    return;
                }

                int gid = get_global_id(0);
                if(gid < count) {
                    float exposure = params[0];
                    float inv_gamma = params[1];
                    
                    pixels[gid] = pixels[gid] * pow(2.0f, exposure);
                    pixels[gid] = pow(pixels[gid], inv_gamma);
                }
            }
        )";  // End of raw string literal

        // Build the program from the kernel source code
        cl::Program::Sources sources;
        sources.push_back(
            {kernelCode.c_str(),
             kernelCode.length() + 1});  // The '+1' is to account for the
                                         // string's null terminator

        cl::Program program(context, sources);
        if (program.build(devices) != CL_SUCCESS) {
            std::cerr << "OpenCL build error: "
                      << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0])
                      << std::endl;
            throw std::runtime_error(
                "OpenCL build error: " +
                program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]));
        }

        // Store the compiled program for later use
        programs["apply_exposure_gamma"] = program;

    } catch (const cl::Error& e) {
        std::cerr << "Exception in ImageProcessor: " << e.what() << " : "
                  << e.err() << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Exception in ImageProcessor: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "An unknown exception occurred in ImageProcessor"
                  << std::endl;
        throw;
    }
}

std::vector<float> ImageProcessor::apply_kernel(
    std::vector<float>& pixels, const std::string& kernel_name,
    const std::vector<float>& parameters) const {
    Timer timer("apply_kernel: " + kernel_name);

    // std::vector<float> modified_pixels = pixels;
    cl_int err;
    // Prepare buffers
    size_t num_pixels = pixels.size();
    cl::Buffer buffer_pixels(context, CL_MEM_READ_WRITE,
                             num_pixels * sizeof(float));
    cl::CommandQueue queue(context);
    err = queue.enqueueWriteBuffer(buffer_pixels, CL_TRUE, 0,
                                   num_pixels * sizeof(float), pixels.data());
    if (err != CL_SUCCESS) {
        // std::cerr << "Error in enqueueWriteBuffer: " << err << "\n";
        // exit(EXIT_FAILURE);
        throw std::runtime_error("Error in enqueueWriteBuffer: " +
                                 std::to_string(err));
    }

    // std::cout << "Applying kernel: " << kernel_name << "\n";
    // std::cout << "Parameter value: " << parameter_value << "\n";

    // Set kernel arguments
    cl::Buffer buffer_params(context, CL_MEM_READ_ONLY,
                             parameters.size() * sizeof(float));
    err = queue.enqueueWriteBuffer(buffer_params, CL_TRUE, 0,
                                   parameters.size() * sizeof(float),
                                   parameters.data());
    if (err != CL_SUCCESS) {
        // std::cerr << "Error in enqueueWriteBuffer for parameters: " << err
        //           << "\n";
        // exit(EXIT_FAILURE);
        throw std::runtime_error("Error in enqueueWriteBuffer: " +
                                 std::to_string(err));
    }

    cl::Program program;
    try {
        program = programs.at(kernel_name);
        // std::cout << "Program: " << program.getInfo<CL_PROGRAM_SOURCE>()
        //           << "\n";
    } catch (const std::out_of_range& e) {
        std::cerr << "Error: Program with key '" << kernel_name
                  << "' not found!\n";
        std::cerr << "Exception: " << e.what() << "\n";
        // Possibly print all existing keys for debugging:
        std::cerr << "Existing keys: ";
        for (const auto& pair : programs) {
            std::cerr << pair.first << " ";
        }
        std::cerr << "\n";
        // exit(EXIT_FAILURE);
        throw std::runtime_error(e.what());
    }

    cl::Kernel kernel(program, kernel_name.c_str());
    kernel.setArg(0, buffer_pixels);
    kernel.setArg(1, static_cast<unsigned int>(num_pixels));
    kernel.setArg(2, buffer_params);
    kernel.setArg(3, static_cast<unsigned int>(parameters.size()));

    // Execute kernel
    // size_t global_work_size = modified_pixels.size();
    err = queue.enqueueNDRangeKernel(kernel, cl::NullRange,
                                     cl::NDRange(num_pixels), cl::NullRange);
    if (err != CL_SUCCESS) {
        std::cerr << "Error in enqueueNDRangeKernel: " << err << "\n";
        exit(EXIT_FAILURE);
    }
    // Retrieve data
    err = queue.enqueueReadBuffer(buffer_pixels, CL_TRUE, 0,
                                  num_pixels * sizeof(float), pixels.data());
    if (err != CL_SUCCESS) {
        // std::cerr << "Error in enqueueReadBuffer: " << err << "\n";
        // exit(EXIT_FAILURE);
        throw std::runtime_error("Error in enqueueReadBuffer: " +
                                 std::to_string(err));
    }
    // Return the modified pixel data
    // return modified_pixels;
    return pixels;
}

std::vector<float> ImageProcessor::apply_gamma_correction(
    std::vector<float>& pixels, float inv_gamma) const {
    return apply_kernel(pixels, "apply_gamma", {inv_gamma});
}

std::vector<float> ImageProcessor::apply_exposure_correction(
    std::vector<float>& pixels, float exposure) const {
    return apply_kernel(pixels, "apply_exposure", {exposure});
}

std::vector<float> ImageProcessor::apply_exposure_gamma_correction(
    std::vector<float>& pixels, float exposure, float inv_gamma) const {
    return apply_kernel(pixels, "apply_exposure_gamma", {exposure, inv_gamma});
}