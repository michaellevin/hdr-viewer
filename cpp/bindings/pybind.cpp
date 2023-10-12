#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // for converting std::vector

#include "../src/image_processing.cpp"

namespace py = pybind11;

PYBIND11_MODULE(hdr_viewer_cpp, m) {
    m.doc() = "Python bindings for hdr-viewer";

    py::class_<DynamicRangeData>(m, "DynamicRangeData")
        .def(py::init<>())
        .def_readwrite("dynamic_range", &DynamicRangeData::dynamic_range)
        .def_readwrite("stops", &DynamicRangeData::stops);

    py::class_<ImageData>(m, "ImageData")
        .def(py::init<>())
        .def_readwrite("pixels", &ImageData::pixels)
        .def_readwrite("dynamic_range_data", &ImageData::dynamic_range_data);

    py::class_<ImageProcessor>(m, "ImageProcessor")
        .def(py::init<>())  // Assuming there is a default constructor
        .def("apply_gamma_correction", &ImageProcessor::apply_gamma_correction,
             "A function to apply gamma correction to image pixels",
             py::arg("pixels"), py::arg("inv_gamma"));

    m.def("scanline_image", [](const std::string& source_path, int new_width) {
        int width, height, channels;
        ImageData image_data =
            scanline_image(source_path, width, height, channels, new_width);
        return pybind11::make_tuple(image_data, width, height, channels);
    });

    m.def("process_image", &process_image,
          "A function to apply gamma to image pixels", py::arg("pixels"),
          py::arg("gamma"));
}
