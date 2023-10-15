#pragma once
#define CL_HPP_TARGET_OPENCL_VERSION 300
#define CL_HPP_ENABLE_EXCEPTIONS 1
#include <CL/opencl.hpp>
#include <map>
#include <string>

class ImageProcessor {
   public:
    ImageProcessor();
    std::vector<float> apply_kernel(const std::vector<float>& pixels,
                                    const std::string& kernel_name,
                                    float parameter_value) const;
    // void apply_gamma_correction(std::vector<float>& pixels, float inv_gamma);
    std::vector<float> apply_gamma_correction(const std::vector<float>& pixels,
                                              float inv_gamma) const;
    std::vector<float> apply_exposure_correction(
        const std::vector<float>& pixels, float exposure) const;

   private:
    cl::Context context;
    // cl::Program program;
    std::map<std::string, cl::Program> programs;
};