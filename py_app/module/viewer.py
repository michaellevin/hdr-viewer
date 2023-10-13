import sys
import os
import math
import numpy as np

# from pathlib import Path
# current_file_path = Path(__file__).resolve()
# current_dir = current_file_path.parent
# relative_path = Path("../../cpp/kernels/gamma_correction.cl")
# target_file_path = (current_dir / relative_path).resolve()
# print(
#     f"The absolute path to the kernel file is: {target_file_path}; Exists: {target_file_path.exists()} "
# )

# import numpy as np
from PySide6.QtWidgets import (
    QApplication,
    QGraphicsScene,
    QGraphicsView,
    QFileDialog,
    QVBoxLayout,
    QHBoxLayout,
    QSlider,
    QLabel,
    QWidget,
    QPushButton,
)
from PySide6.QtGui import QImage, QPixmap
from PySide6.QtCore import Qt

# folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "extensions")
# sys.path.append(folder)

current_file_path = os.path.abspath(__file__)
current_dir = os.path.dirname(current_file_path)
extensions_folder = os.path.join(current_dir, "..", "extensions")
extensions_folder = os.path.abspath(extensions_folder)
print(f"Extensions_folder: {extensions_folder}")
sys.path.append(extensions_folder)

import hdr_viewer_cpp as hdr_viewer


def format_dynamic_range(dynamic_range):
    if dynamic_range >= 1e6:
        return f"{dynamic_range/1e6:.2f}M"
    elif dynamic_range >= 1e3:
        return f"{dynamic_range/1e3:.2f}K"
    else:
        return f"{dynamic_range:.2f}"


def format_stops(stops):
    return f"{stops:.1f}"


class ImageViewer(QWidget):
    def __init__(self, image_path=None):
        super().__init__()
        self.image_path = image_path
        self.image_data = None
        self.width = 0
        self.height = 0
        self.channels = 0
        self.image_processor = hdr_viewer.ImageProcessor()
        self.init_ui()
        if self.image_path:
            self.load_image(self.image_path)

    def init_ui(self):
        self._setup_widgets()
        self._setup_layout()
        self._connect_signals()

        self.setWindowTitle("HDR/EXR Image Viewer")
        self.setGeometry(100, 100, 800, 600)

    def _setup_widgets(self):
        self.scene = QGraphicsScene(self)
        self.view = QGraphicsView(self.scene)
        self.load_button = QPushButton("Load Image", self)
        self.info_label = QLabel("", self)

        # Gamma widgets
        self.gamma_slider = QSlider(Qt.Horizontal, self)
        self.gamma_slider.setMinimum(5)
        self.gamma_slider.setMaximum(50)
        self.gamma_slider.setValue(22)
        self.gamma_label = QLabel("Gamma 2.2", self)

        # Exposure widgets
        self.exposure_slider = QSlider(Qt.Horizontal, self)
        self.exposure_slider.setMinimum(-20)
        self.exposure_slider.setMaximum(20)
        self.exposure_slider.setValue(0)
        self.exposure_label = QLabel("Exposure: 0", self)

    def _setup_layout(self):
        gamma_layout = QHBoxLayout()
        gamma_layout.addWidget(self.gamma_label)
        gamma_layout.addWidget(self.gamma_slider)

        exposure_layout = QHBoxLayout()
        exposure_layout.addWidget(self.exposure_label)
        exposure_layout.addWidget(self.exposure_slider)

        layout = QVBoxLayout(self)
        layout.addWidget(self.load_button)
        layout.addWidget(self.info_label)
        layout.addLayout(gamma_layout)
        layout.addLayout(exposure_layout)
        layout.addWidget(self.view)
        self.setLayout(layout)

    def _connect_signals(self):
        self.load_button.clicked.connect(self.open_image_dialog)
        self.gamma_slider.valueChanged.connect(self.update_gamma)
        self.exposure_slider.valueChanged.connect(self.update_exposure)

    def load_image(self, fname):
        (
            self.image_data,
            self.width,
            self.height,
            self.channels,
        ) = hdr_viewer.scanline_image(fname, 1024)
        self.update_image()
        dynamic_range = format_dynamic_range(
            self.image_data.dynamic_range_data.dynamic_range
        )
        stops = format_stops(self.image_data.dynamic_range_data.stops)
        self.info_label.setText(
            f"Image: {fname} - {self.width} x {self.height} x {self.channels} "
            f"- Dynamic range: {dynamic_range}, stops: {stops}"
        )
        self.update_gamma()

    def open_image_dialog(self):
        fname, _ = QFileDialog.getOpenFileName(
            self, "Open file", ".", "Image files (*.exr *.hdr)"
        )
        if fname:
            self.load_image(fname)

    def update_gamma(self):
        gamma_value = self.gamma_slider.value() / 10.0
        self.gamma_label.setText(f"Gamma: {gamma_value:.1f}")
        inv_gamma = 1.0 / gamma_value
        processed_image_data = self.image_processor.apply_gamma_correction(
            self.image_data.pixels, inv_gamma
        )
        self.display_image(processed_image_data)

    def update_exposure(self):
        exposure_value = self.exposure_slider.value() / 10.0
        self.exposure_label.setText(f"Exposure: {exposure_value:.1f}")
        processed_image_data = self.image_processor.apply_exposure_correction(
            self.image_data.pixels, exposure_value
        )
        self.display_image(processed_image_data)

    def display_image(self, img_data):
        img_data_np = np.array(img_data)
        int_values = np.clip(img_data_np * 255, 0, 255).astype(np.uint8)
        byte_array = bytes(int_values.tobytes())
        q_img = QImage(byte_array, self.width, self.height, QImage.Format_RGBA8888)

        pixmap = QPixmap(q_img)
        self.scene.clear()
        self.scene.addPixmap(pixmap)
        self.view.fitInView(self.scene.sceneRect(), Qt.KeepAspectRatio)

    def update_image(self):
        self.display_image(self.image_data.pixels)


if __name__ == "__main__":
    hdr_file = os.path.join(
        os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
        "examples",
        "HDR_001.exr",
    )
    print(f"hdr_file: {hdr_file}")
    # image_pixels, orig_width, orig_height, channels = hdr_viewer.scanline_image(
    #     hdr_file, 1024
    # )
    # print(orig_width, orig_height, channels)

    app = QApplication(sys.argv)
    viewer = ImageViewer(hdr_file)
    viewer.show()
    sys.exit(app.exec())
