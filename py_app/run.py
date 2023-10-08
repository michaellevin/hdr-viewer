import sys
import os

# folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "extensions")
# sys.path.append(folder)
folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "extensions")
sys.path.append(folder)

import hdr_viewer_cpp as hdr_viewer

width = 0
height = 0

hdr_file = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "examples",
    "HDR_001.exr",
)

image_pixels, orig_width, orig_height, channels = hdr_viewer.scanline_image(
    hdr_file, 1024
)
print(orig_width, orig_height, channels)
