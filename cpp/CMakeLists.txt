cmake_minimum_required(VERSION 3.8)

# Project Information
project(
    ImageProcessing 
    VERSION 0.3.2 
    LANGUAGES CXX
)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Set vcpkg toolchain file > moved to .vscode/settings.json
# set(CMAKE_TOOLCHAIN_FILE "[vcpkg root]/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
if(UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
set(CMAKE_VERBOSE_MAKEFILE ON) # Print verbose makefile commands

# Find the packages
find_package(OpenImageIO CONFIG REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
find_package(OpenCL REQUIRED)

# file(GLOB SOURCES "src/*.cpp") # Specify the executable and its source files. 
set(SOURCES
    src/image_io.cpp
    src/image_processing.cpp
    src/timer.cpp
    # Add other source files here
)
add_library(image_processing_lib STATIC ${SOURCES})
target_include_directories(image_processing_lib PUBLIC include)
if(APPLE)
    add_compile_definitions(USE_OPENCL_120)
    # Mac: find the base directory containing 'CL/opencl.hpp'
    if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
        message("Detected Apple Silicon (ARM architecture).")
        execute_process(COMMAND find /opt/homebrew/Cellar/opencl-clhpp-headers 
                        -name "opencl.hpp" OUTPUT_VARIABLE OpenCL_INCLUDE_DIR)
        get_filename_component(OpenCL_INCLUDE_DIR ${OpenCL_INCLUDE_DIR} DIRECTORY) # get Parent folder
        get_filename_component(OpenCL_INCLUDE_DIR ${OpenCL_INCLUDE_DIR} DIRECTORY) # get Parent folder
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
        message("Detected Intel architecture.")
        get_filename_component(OpenCL_INCLUDE_DIR ${OpenCL_INCLUDE_DIR} DIRECTORY) # get Parent folder
    else()
        message("Detected other architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
        get_filename_component(OpenCL_INCLUDE_DIR ${OpenCL_INCLUDE_DIR} DIRECTORY) # get Parent folder
    endif()
    if(NOT OpenCL_INCLUDE_DIR)
        message(FATAL_ERROR "OpenCL include path not found!")
    endif()
    message(STATUS "[${CMAKE_SYSTEM_NAME}] OpenCL include directory: ${OpenCL_INCLUDE_DIR}")
    # Include the directory and ensure it's used for this target
    target_include_directories(image_processing_lib SYSTEM PUBLIC ${OpenCL_INCLUDE_DIR})
else()
    # Win/Linux
    message(STATUS "[${CMAKE_SYSTEM_NAME}] OpenCL include directory: ${OpenCL_INCLUDE_DIR}")
    target_include_directories(image_processing_lib SYSTEM PUBLIC ${OpenCL_INCLUDE_DIR})
endif()

# target_include_directories(image_processing_lib PUBLIC ${OpenCL_INCLUDE_DIRS})
target_link_libraries(image_processing_lib PRIVATE OpenImageIO::OpenImageIO ${OpenCL_LIBRARIES})  # Link against OpenCL

add_executable(Image_Processing src/main.cpp)
target_link_libraries(Image_Processing PRIVATE image_processing_lib)

add_library(hdr_viewer_cpp MODULE bindings/pybind.cpp)
if(WIN32)
    set_target_properties(hdr_viewer_cpp PROPERTIES SUFFIX ".pyd")
else()
    # The default suffix for MODULE libraries in CMake is .so
endif()
target_link_libraries(hdr_viewer_cpp PRIVATE image_processing_lib pybind11::module)