cmake_minimum_required(VERSION 3.8)

# Project Information
project(
    ImageProcessing 
    VERSION 0.2.1 
    LANGUAGES CXX
)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set vcpkg toolchain file > moved to .vscode/settings.json
# set(CMAKE_TOOLCHAIN_FILE "[vcpkg root]/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Find the packages
find_package(OpenImageIO CONFIG REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

# * Build executable (main)
# file(GLOB SOURCES "src/*.cpp") # Specify the executable and its source files. Including all .cpp files in the src directory.
set(SOURCES
    src/image_processing.cpp
    src/timer.cpp
    # Add other source files here
)
add_executable(Image_Processing ${SOURCES})
# add_executable(Image_Processing src/image_processing.cpp)
target_include_directories(Image_Processing PRIVATE include) # Include directory for header files
target_link_libraries(Image_Processing PRIVATE OpenImageIO::OpenImageIO) # Link OpenImageIO to your executable

# Build library for python bindings
add_library(hdr_viewer_cpp MODULE bindings/pybind.cpp)
target_link_libraries(hdr_viewer_cpp PRIVATE pybind11::module OpenImageIO) # # Linking the libraries needed (if any)
target_compile_features(hdr_viewer_cpp PRIVATE cxx_std_17)
target_include_directories(hdr_viewer_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include include) # Additional include directories
# Optional: Add compile options for better code quality
# target_compile_options(Image_Processing PRIVATE -Wall -Wextra)