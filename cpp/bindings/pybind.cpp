#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // for converting std::vector
#include "../src/image_processing.cpp"

namespace py = pybind11;

PYBIND11_MODULE(hdr_viewer_cpp, m) {
    m.doc() = "Python bindings for hdr-viewer";

    m.def("scanline_image", &scanline_image,
            "A function that scans an image and returns its pixel data along with width, height, and channels",
            py::arg("source_path"),
            py::arg("width"),
            py::arg("height"),
            py::arg("channels"),
            py::arg("new_width")
    );

    m.def("process_image", &process_image,
            "A function that processes image pixels",
            py::arg("pixels"),
            py::arg("width"),
            py::arg("height"),
            py::arg("channels"),
            py::arg("gamma")
    );
}
