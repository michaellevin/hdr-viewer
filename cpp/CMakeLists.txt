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

# file(GLOB SOURCES "src/*.cpp") # Specify the executable and its source files. Including all .cpp files in the src directory.
set(SOURCES
    src/image_io.cpp
    src/image_processing.cpp
    src/timer.cpp
    # Add other source files here
)
add_library(image_processing_lib STATIC ${SOURCES})
target_include_directories(image_processing_lib PUBLIC include)
message(STATUS "OpenCL include directories: ${OpenCL_INCLUDE_DIRS}")
target_include_directories(image_processing_lib PUBLIC ${OpenCL_INCLUDE_DIRS})
target_link_libraries(image_processing_lib PRIVATE OpenImageIO::OpenImageIO ${OpenCL_LIBRARIES})  # Link against OpenCL

add_executable(Image_Processing src/main.cpp)
target_link_libraries(Image_Processing PRIVATE image_processing_lib)

add_library(hdr_viewer_cpp MODULE bindings/pybind.cpp)
if(WIN32)
    set_target_properties(hdr_viewer_cpp PROPERTIES SUFFIX ".pyd")
else()
    # The default suffix for MODULE libraries in CMake is .so, which is correct for Python extensions on Unix
    # No need to explicitly set it unless you need a non-standard suffix or additional behavior
endif()
target_link_libraries(hdr_viewer_cpp PRIVATE image_processing_lib pybind11::module)