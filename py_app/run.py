import sys
import os

release_folder = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "Release"
)
sys.path.append(release_folder)
from pprint import pprint

pprint(sys.path)
import hdr_viewer_cpp as hdr_viewer
