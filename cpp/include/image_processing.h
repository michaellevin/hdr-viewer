#pragma once
#ifdef USE_OPENCL_120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#else
#define CL_HPP_TARGET_OPENCL_VERSION 300
#endif
#define CL_HPP_ENABLE_EXCEPTIONS 1
// #include <CL/opencl.hpp>
if __has_include (<CL / opencl.hpp>)
#include <CL/opencl.hpp>
#else
#include <opencl.h>
#endif

#include <map>
#include <string>

    class ImageProcessor {
       public:
        ImageProcessor();
        std::vector<float> apply_kernel(
            std::vector<float>& pixels, const std::string& kernel_name,
            const std::vector<float>& parameters) const;
        // void apply_gamma_correction(std::vector<float>& pixels, float
        // inv_gamma);
        std::vector<float> apply_gamma_correction(std::vector<float>& pixels,
                                                  float inv_gamma) const;
        std::vector<float> apply_exposure_correction(std::vector<float>& pixels,
                                                     float exposure) const;
        std::vector<float> apply_exposure_gamma_correction(
            std::vector<float>& pixels, float exposure, float inv_gamma) const;

       private:
        cl::Context context;
        // cl::Program program;
        std::map<std::string, cl::Program> programs;
    };