import sys
import os
import math

# import numpy as np
from PySide6.QtWidgets import (
    QApplication,
    QGraphicsScene,
    QGraphicsView,
    QFileDialog,
    QVBoxLayout,
    QSlider,
    QLabel,
    QWidget,
    QPushButton,
)
from PySide6.QtGui import QImage, QPixmap
from PySide6.QtCore import Qt

folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "extensions")
sys.path.append(folder)

import hdr_viewer_cpp as hdr_viewer


class ImageViewer(QWidget):
    def __init__(self):
        super().__init__()
        self.init_ui()

    def init_ui(self):
        self.scene = QGraphicsScene(self)
        self.view = QGraphicsView(self.scene)
        self.slider = QSlider(Qt.Horizontal, self)
        self.load_button = QPushButton("Load Image", self)
        self.gamma_label = QLabel("Gamma: 2.2", self)

        layout = QVBoxLayout(self)
        layout.addWidget(self.load_button)
        layout.addWidget(self.gamma_label)
        layout.addWidget(self.slider)
        layout.addWidget(self.view)

        self.slider.setMinimum(1)
        self.slider.setMaximum(80)  # Adjust as needed
        self.slider.setValue(22)

        self.load_button.clicked.connect(self.load_image)
        self.slider.valueChanged.connect(self.update_gamma)

        self.setWindowTitle("HDR/EXR Image Viewer")
        self.setGeometry(100, 100, 800, 600)

    def load_image(self):
        fname, _ = QFileDialog.getOpenFileName(
            self, "Open file", ".", "Image files (*.exr *.hdr)"
        )
        if fname:
            (
                self.image_data,
                self.width,
                self.height,
                self.channels,
            ) = hdr_viewer.scanline_image(fname, 1024)
            self.update_image()

    def update_gamma(self):
        gamma_value = self.slider.value() / 10.0
        self.gamma_label.setText(f"Gamma: {gamma_value:.1f}")
        processed_image_data = hdr_viewer.process_image(self.image_data, gamma_value)
        self.display_image(processed_image_data)

    def update_image(self):
        self.display_image(self.image_data)

    def display_image(self, img_data):
        # print(img_data)
        # np_img_data = np.array(img_data, dtype=np.float32).reshape(
        #     (self.height, self.width, self.channels)
        # )
        # # Convert to 8-bit pixel values
        # np_img_data = np.clip(np_img_data * 255, 0, 255).astype(np.uint8)
        # height, width, channel = np_img_data.shape
        # bytes_per_line = 3 * width
        # q_img = QImage(
        #     np_img_data.data, width, height, bytes_per_line, QImage.Format_RGB888
        # )

        int_values = [
            min(255, max(0, int(x * 255))) if not math.isnan(x) else 0 for x in img_data
        ]
        byte_array = bytes(int_values)
        # Create a QImage from the byte array.
        q_img = QImage(byte_array, self.width, self.height, QImage.Format_RGBA8888)

        pixmap = QPixmap(q_img)
        self.scene.clear()
        self.scene.addPixmap(pixmap)
        self.view.fitInView(self.scene.sceneRect(), Qt.KeepAspectRatio)


if __name__ == "__main__":
    hdr_file = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "examples",
        "HDR_001.exr",
    )

    # image_pixels, orig_width, orig_height, channels = hdr_viewer.scanline_image(
    #     hdr_file, 1024
    # )
    # print(orig_width, orig_height, channels)

    app = QApplication(sys.argv)
    viewer = ImageViewer()
    viewer.show()
    sys.exit(app.exec())
