#!/usr/bin/env python3
"""Cuts the app icon out of the artwork sheet and writes every size the RPM
ships.

The source is a sheet of three proposals on an opaque background; what the app
needs is one of them, square, with a real alpha channel. The interesting part
is the alpha, and it is derived from the artwork rather than reconstructed:
the outline is a rounded shape whose corners are not circular arcs, so any
mask drawn by hand would be visibly wrong along the curve.

So the shape is recovered from the pixels instead. The background is a pale
neutral and the icon is saturated, which separates them everywhere except
inside the white glyph - and that is enclosed by icon colour on all sides, so
filling the holes puts it back. A small erosion then pulls the edge inside the
JPEG's own fringe, which is where the compression artefacts and the drop
shadow live.

    docker compose run --rm icons
"""

import sys
from collections import deque
from pathlib import Path

try:
    from PIL import Image, ImageFilter
except ImportError:
    print("Pillow is required: pip install pillow")
    sys.exit(1)

ROOT = Path(__file__).resolve().parent.parent
ICON_DIR = ROOT / "icons"
SOURCE = ICON_DIR / "source-artwork.jpg"

SIZES = [86, 108, 128, 172]

# Proposal B ("Réseau Local") on the sheet, found by scanning for saturated
# pixels. Deliberately loose: the shape is trimmed to its own mask afterwards,
# so this only has to contain the icon and none of its neighbours. Cropping
# tight here instead would clip the two sharp corners, which sit exactly on
# the boundary that the scan reports.
CROP = (365, 333, 655, 625)

# Above this, a pixel is icon rather than background. The gap between the pale
# background and the artwork is wide, so the exact value is not delicate.
SATURATION_THRESHOLD = 0.20

# Pulls the edge inside the JPEG's halo. Two pixels at source scale is well
# under one pixel at every size we ship.
ERODE_PIXELS = 2


def saturation(pixel):
    red, green, blue = [channel / 255.0 for channel in pixel[:3]]
    high = max(red, green, blue)
    low = min(red, green, blue)
    return 0.0 if high == 0 else (high - low) / high


def shape_mask(image):
    """A binary mask of the icon, glyph included."""
    width, height = image.size
    pixels = image.load()

    solid = bytearray(width * height)
    for y in range(height):
        row = y * width
        for x in range(width):
            if saturation(pixels[x, y]) > SATURATION_THRESHOLD:
                solid[row + x] = 1

    # Everything reachable from the border without crossing the icon is
    # outside it. What is left over is the icon plus the white glyph, which is
    # unreachable precisely because the icon encloses it.
    outside = bytearray(width * height)
    queue = deque()

    for x in range(width):
        for y in (0, height - 1):
            if not solid[y * width + x] and not outside[y * width + x]:
                outside[y * width + x] = 1
                queue.append((x, y))
    for y in range(height):
        for x in (0, width - 1):
            if not solid[y * width + x] and not outside[y * width + x]:
                outside[y * width + x] = 1
                queue.append((x, y))

    while queue:
        x, y = queue.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if nx < 0 or ny < 0 or nx >= width or ny >= height:
                continue
            index = ny * width + nx
            if solid[index] or outside[index]:
                continue
            outside[index] = 1
            queue.append((nx, ny))

    mask = Image.new("L", (width, height), 0)
    mask.putdata([0 if flag else 255 for flag in outside])
    return mask


def mean_colour(image, mask):
    """Average colour of the icon, used to flood the area outside it.

    Resizing an image whose transparent region is a pale background pulls that
    background into the edge pixels, and the result is a light halo around the
    shape at every size. Filling the outside with the icon's own colour first
    means the blend at the edge is icon against icon.
    """
    pixels = image.load()
    flags = mask.load()
    width, height = image.size

    totals = [0, 0, 0]
    count = 0
    for y in range(0, height, 2):
        for x in range(0, width, 2):
            if flags[x, y] < 128:
                continue
            pixel = pixels[x, y]
            totals[0] += pixel[0]
            totals[1] += pixel[1]
            totals[2] += pixel[2]
            count += 1

    if count == 0:
        return (0, 0, 0)
    return tuple(total // count for total in totals)


def build_master():
    if not SOURCE.exists():
        print("missing %s" % SOURCE.relative_to(ROOT))
        return None

    sheet = Image.open(SOURCE).convert("RGB")
    art = sheet.crop(CROP)

    mask = shape_mask(art)
    # MinFilter is an erosion: each pixel takes the darkest of its
    # neighbourhood, so the mask shrinks by half the window on every side.
    mask = mask.filter(ImageFilter.MinFilter(ERODE_PIXELS * 2 + 1))

    # Trim to what the mask actually covers. Doing it here rather than by
    # measuring the sheet means the two sharp corners end up exactly on the
    # canvas edge however loose the crop above was.
    bounds = mask.getbbox()
    if bounds is None:
        print("no icon found in the crop")
        return None
    art = art.crop(bounds)
    mask = mask.crop(bounds)

    flooded = Image.new("RGB", art.size, mean_colour(art, mask))
    flooded.paste(art, (0, 0), mask)

    master = flooded.convert("RGBA")
    master.putalpha(mask)

    # Padded rather than cropped to square: the shape is square to within a
    # pixel or two, and trimming to the shorter side would take the tip off a
    # corner to fix a rounding error.
    side = max(master.size)
    square = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    square.paste(master, ((side - master.size[0]) // 2,
                          (side - master.size[1]) // 2))
    return square


def main():
    master = build_master()
    if master is None:
        return 1

    for size in SIZES:
        directory = ICON_DIR / ("%dx%d" % (size, size))
        directory.mkdir(parents=True, exist_ok=True)
        path = directory / "harbour-localsend.png"

        # Colour and alpha are resized together here, which is safe because
        # the colour outside the shape was replaced with the icon's own.
        master.resize((size, size), Image.LANCZOS).save(path)
        print("wrote %s" % path.relative_to(ROOT))

    return 0


if __name__ == "__main__":
    sys.exit(main())
