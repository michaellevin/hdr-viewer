#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // for converting std::vector

#include "../src/image_io.cpp"
#include "../src/image_processing.cpp"

namespace py = pybind11;

PYBIND11_MODULE(hdr_viewer_cpp, m) {
    m.doc() = "Python bindings for hdr-viewer";

    py::class_<DynamicRangeData>(m, "DynamicRangeData")
        .def(py::init<>())
        .def_readwrite("dynamic_range", &DynamicRangeData::dynamic_range)
        .def_readwrite("stops", &DynamicRangeData::stops);

    py::class_<ImageData>(m, "ImageData")
        .def(py::init<>())  // If you have a default constructor
        .def_readwrite("pixels", &ImageData::pixels)
        .def_readwrite("original_width", &ImageData::original_width)
        .def_readwrite("original_height", &ImageData::original_height)
        .def_readwrite("num_original_channels",
                       &ImageData::num_original_channels)
        .def_readwrite("resized_width", &ImageData::resized_width)
        .def_readwrite("resized_height", &ImageData::resized_height)
        .def_readwrite("num_output_channels", &ImageData::num_output_channels)
        .def_readwrite("original_has_alpha", &ImageData::original_has_alpha)
        .def_readwrite("output_has_alpha", &ImageData::output_has_alpha)
        .def_property_readonly(
            "dynamic_range_data",
            [](const ImageData& self) -> const DynamicRangeData* {
                return self.dynamic_range_data.get();
            })
        .def("hasDynamicRangeData", &ImageData::hasDynamicRangeData);

    py::class_<ImageProcessor>(m, "ImageProcessor")
        .def(py::init<>())  // Assuming there is a default constructor
        .def("apply_gamma_correction", &ImageProcessor::apply_gamma_correction,
             "A function to apply gamma correction to image pixels",
             py::arg("pixels"), py::arg("inv_gamma"))
        .def("apply_exposure_correction",
             &ImageProcessor::apply_exposure_correction,
             "A function to apply exposure correction to image pixels",
             py::arg("pixels"), py::arg("exposure"))
        .def(
            "apply_exposure_gamma_correction",
            &ImageProcessor::apply_exposure_gamma_correction,
            "A function to apply exposure and gamma correction to image pixels",
            py::arg("pixels"), py::arg("exposure"), py::arg("inv_gamma"));
    m.def("scanline_image", [](const std::string& source_path, int new_width) {
        ImageData image_data = scanline_image(source_path, new_width);
        return image_data;
    });

    // m.def("process_image", &process_image,
    //       "A function to apply gamma to image pixels", py::arg("pixels"),
    //       py::arg("gamma"));
}
