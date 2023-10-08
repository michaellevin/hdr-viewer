#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // for converting std::vector
#include "../src/image_processing.cpp"

namespace py = pybind11;

PYBIND11_MODULE(hdr_viewer_cpp, m) {
        m.doc() = "Python bindings for hdr-viewer";

        // m.def("scanline_image", &scanline_image,
        // "A function that scans an image and returns its pixel data along with width, height, and channels",
        // py::arg("source_path"),
        // py::arg("width"),
        // py::arg("height"),
        // py::arg("channels"),
        // py::arg("new_width")
        // );
        py::object ImageData = py::module_::import("collections").attr("namedtuple")("ImageData", "pixels width height channels");

        m.def("scanline_image", [](const std::string& source_path, int new_width) {
                int width, height, channels;
                std::vector<float> pixels = scanline_image(source_path, width, height, channels, new_width);
                return pybind11::make_tuple(pixels, width, height, channels);
                // return ImageData(py::make_tuple(pixels, width, height, channels));
        });

        m.def("process_image", &process_image,
        "A function to apply gamma to image pixels",
        py::arg("pixels"),
        py::arg("gamma")
        );
}
