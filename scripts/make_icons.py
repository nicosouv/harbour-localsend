#!/usr/bin/env python3
"""Draws the app icon at every size the RPM ships.

The artwork is code rather than a checked-in master image, because the icon is
geometry: rings and an arrow, positioned in fractions of the canvas. At 86
pixels a scaled-down 512-pixel PNG turns to mush, while re-drawing at the
target size keeps the strokes on whole pixels.

Everything is drawn at eight times the final size and then reduced, which is
how you get antialiasing out of ImageDraw - it has none of its own.

    docker compose run --rm icons
"""

import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("Pillow is required: pip install pillow")
    sys.exit(1)

ROOT = Path(__file__).resolve().parent.parent
ICON_DIR = ROOT / "icons"

SIZES = [86, 108, 128, 172]
SUPERSAMPLE = 8

# Harbour water, top to bottom. Dark enough that the rings read as light.
TOP = (24, 58, 76)
BOTTOM = (18, 94, 122)

# The rings and the dot: the same pale cyan the UI uses for its own radar.
RING = (208, 240, 248)
# The arrow leaving the rings, warm so it separates from everything behind it.
ARROW = (255, 198, 88)


def gradient(size):
    image = Image.new("RGB", (size, size))
    pixels = image.load()
    for y in range(size):
        ratio = y / max(1, size - 1)
        colour = tuple(int(TOP[i] + (BOTTOM[i] - TOP[i]) * ratio)
                       for i in range(3))
        for x in range(size):
            pixels[x, y] = colour
    return image


def rounded_mask(size):
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle([0, 0, size - 1, size - 1],
                           radius=int(size * 0.225), fill=255)
    return mask


def draw_icon(size):
    canvas = size * SUPERSAMPLE

    image = gradient(canvas).convert("RGBA")
    overlay = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    # The radar sits low and left, leaving the upper right for the arrow.
    centre_x = canvas * 0.40
    centre_y = canvas * 0.62
    stroke = max(1, int(canvas * 0.030))

    # Rings fade outwards, the same way the ones in the app do.
    for index, radius_ratio in enumerate([0.155, 0.255, 0.355]):
        radius = canvas * radius_ratio
        alpha = int(235 - index * 65)
        draw.ellipse(
            [centre_x - radius, centre_y - radius,
             centre_x + radius, centre_y + radius],
            outline=RING + (alpha,), width=stroke)

    dot = canvas * 0.055
    draw.ellipse([centre_x - dot, centre_y - dot,
                  centre_x + dot, centre_y + dot],
                 fill=RING + (255,))

    # The arrow: a shaft out of the centre to the upper right, and a solid
    # head. The head is built from the shaft's own direction rather than from
    # hand-placed offsets - eyeballed barbs only look square at one angle, and
    # they were not square at this one.
    tip_x = canvas * 0.80
    tip_y = canvas * 0.22
    shaft = max(1, int(canvas * 0.042))

    span_x = tip_x - centre_x
    span_y = tip_y - centre_y
    length = (span_x ** 2 + span_y ** 2) ** 0.5
    unit_x = span_x / length
    unit_y = span_y / length
    # Left-hand normal of the direction vector.
    normal_x = -unit_y
    normal_y = unit_x

    head_length = canvas * 0.19
    head_width = canvas * 0.085

    # The shaft stops inside the head so its squared-off end never pokes
    # through the point.
    base_x = tip_x - unit_x * head_length * 0.75
    base_y = tip_y - unit_y * head_length * 0.75
    draw.line([centre_x, centre_y, base_x, base_y],
              fill=ARROW + (255,), width=shaft)

    back_x = tip_x - unit_x * head_length
    back_y = tip_y - unit_y * head_length
    draw.polygon([
        (tip_x, tip_y),
        (back_x + normal_x * head_width, back_y + normal_y * head_width),
        (back_x - normal_x * head_width, back_y - normal_y * head_width),
    ], fill=ARROW + (255,))

    image = Image.alpha_composite(image, overlay)
    image.putalpha(rounded_mask(canvas))

    return image.resize((size, size), Image.LANCZOS)


def main():
    for size in SIZES:
        directory = ICON_DIR / f"{size}x{size}"
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / "harbour-localsend.png"
        draw_icon(size).save(path)
        print(f"wrote {path.relative_to(ROOT)}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
