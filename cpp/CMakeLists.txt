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

# Find the OpenImageIO package
find_package(OpenImageIO CONFIG REQUIRED)

# Specify the executable and its source files
# Including all .cpp files in the src directory.
file(GLOB SOURCES "src/*.cpp")
add_executable(Image_Processing ${SOURCES})
# add_executable(Image_Processing src/image_processing.cpp)

# Include directory for header files
target_include_directories(Image_Processing PRIVATE include)

# Link OpenImageIO to your executable
target_link_libraries(Image_Processing PRIVATE OpenImageIO::OpenImageIO)

# Optional: Add compile options for better code quality
# target_compile_options(Image_Processing PRIVATE -Wall -Wextra)