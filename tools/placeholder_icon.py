#!/usr/bin/env python3
"""A plain placeholder app icon (no artwork of anyone's): placeholder_icon.py <out.png> [size]"""
import sys
from PIL import Image, ImageDraw

out = sys.argv[1]
n = int(sys.argv[2]) if len(sys.argv) > 2 else 152
im = Image.new("RGBA", (n, n), (18, 24, 48, 255))
d = ImageDraw.Draw(im)
d.rectangle((0, int(n * 0.72), n, n), fill=(200, 40, 40, 255))
d.rectangle((int(n * 0.18), int(n * 0.30), int(n * 0.82), int(n * 0.46)), fill=(235, 235, 240, 255))
im.save(out)
