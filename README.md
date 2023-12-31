# Universal Image Viewer with HDR Support
## Overview
Universal Image Viewer is a versatile tool designed for quick and efficient viewing of images across various formats using OpenImageIO and OpenCL. While it's engineered to handle a wide spectrum of image types, it boasts specialized features for high-dynamic-range (HDR) images.

## Features
- Broad Format Support: Opens and displays images in multiple formats such as HDR, EXR, TIFF, PNG, JPG etc.
- Exposure Adjustment
- Gamma Correction
- Real-Time Performance: Optimized for speed, providing a smooth, real-time experience when applying adjustments and browsing through images.

## Installation
Follow these instructions to set up the project on your local machine for development and testing purposes.

### Prerequisites
- Git
- CMake (3.8 or higher)
- Python (3.9 or higher)
- Homebrew (for macOS users)
- vcpkg (for Windows users)

### Windows Setup
1. **Install vcpkg:** We use vcpkg to manage our C++ dependencies. To install it, clone the vcpkg repository and run the bootstrap script as follows:
```bash
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
```
2. **Integrate vcpkg with your system:** Run the following command:

```bash
.\vcpkg\vcpkg integrate install
```
This makes the installed libraries globally available to your build systems.

3. **Install dependencies:** Use vcpkg to install the necessary packages:

```bash
.\vcpkg\vcpkg install openimageio:x64-windows pybind11:x64-windows opencl:x64-windows
```

4. **Environment Variable:** It is recommended to set an environment variable for VCPKG_ROOT:

```bash
setx VCPKG_ROOT <path_to_vcpkg_installation>
```

5. **CMake Configuration:** While setting up your project with CMake, add the vcpkg entry to your toolchain settings to make sure CMake uses the vcpkg toolchain:

```bash
cmake -E make_directory ./build
cmake ./cpp -B ./build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=${env:VCPKG_ROOT}/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## macOS Setup

**OpenCL:** macOS comes with OpenCL pre-installed, but it's deprecated and will be phased out for Metal. Currently, we are constrained to use OpenCL 1.2 (as opposed to 3.0 on Windows).

**Install dependencies:** You can use Homebrew to install the necessary packages:

```bash
brew install openimageio
brew install pybind11
brew install opencl-clhpp-headers
```

**CMake Configuration for Older macOS Versions:** If you're building on an older version of macOS, ensure you set the deployment target for compatibility. 

```bash
cmake -E make_directory ./build
cmake ./cpp -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

If you need your application to support older versions of macOS, you can specify the minimum OS version required for your project by adding a flag to your CMake configuration command. For example, to support macOS 11.6 or newer, you would use:
```bash
cmake .. -DCMAKE_OSX_DEPLOYMENT_TARGET=11.6
```


## Python setup
For the Python part, you need to install dependencies via pip:

```bash
pip install -r requirements.txt
```
TBC