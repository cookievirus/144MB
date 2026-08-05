#!/usr/bin/env python3
"""Slice HERO-BEST-MOOD-01 into a vertical portrait strip.

The sheet is not on an even grid: nominal 5x4 cells put the shoulders of one
row inside the top of the next, so an arithmetic slice bleeds. Row and column
boundaries are found from content density instead, then each face is squared
off anchored to the top of its hair.

All frames share one image so they share one 16-colour palette and one GPU
texture; the runtime picks a source rect by mood index.
"""
import sys

import numpy as np
from PIL import Image

SIZE = 48
DARK = 60          # summed-RGB level below which a pixel counts as background


def gaps(density, min_run=5, thresh=0.02):
    """Index ranges where the sheet is effectively empty: seams and margins.

    The threshold is 2% rather than 0 because the seams carry a little
    compression noise from the source render.
    """
    empty = density <= thresh
    out, start = [], None
    for i, e in enumerate(empty):
        if e and start is None:
            start = i
        elif not e and start is not None:
            if i - start >= min_run:
                out.append((start, i))
            start = None
    if start is not None and len(empty) - start >= min_run:
        out.append((start, len(empty)))
    return out


def spans(density, count):
    """Content bands, i.e. everything the gaps leave behind."""
    bands, cursor = [], 0
    for a, b in gaps(density):
        if a > cursor:
            bands.append((cursor, a))
        cursor = b
    if cursor < len(density):
        bands.append((cursor, len(density)))

    # Stray pixels at the sheet margin can survive as a sliver band; anything
    # far narrower than a real cell is noise, not a face.
    if bands:
        widest = max(b - a for a, b in bands)
        bands = [(a, b) for a, b in bands if (b - a) * 3 >= widest]

    if len(bands) != count:
        raise SystemExit("  ! found %d bands, expected %d: %s"
                         % (len(bands), count, bands))
    return bands


def main():
    src, dst = sys.argv[1], sys.argv[2]
    cols, rows = 5, 4

    sheet = Image.open(src).convert("RGB")
    lum = np.asarray(sheet, int).sum(axis=2)
    solid = lum > DARK

    row_bands = spans(solid.mean(axis=1), rows)
    col_bands = spans(solid.mean(axis=0), cols)
    print("  rows: %s" % row_bands, file=sys.stderr)
    print("  cols: %s" % col_bands, file=sys.stderr)

    strip = Image.new("RGB", (SIZE, SIZE * cols * rows))
    for r, (y0, y1) in enumerate(row_bands):
        for c, (x0, x1) in enumerate(col_bands):
            cell = solid[y0:y1, x0:x1]
            ys, xs = np.nonzero(cell)
            cy0, cx0, cx1 = y0 + ys.min(), x0 + xs.min(), x0 + xs.max() + 1

            # Square, anchored to the top of the hair: the face is what
            # matters and the shoulders can fall off the bottom edge.
            side = cx1 - cx0
            mid = (cx0 + cx1) // 2
            box = (mid - side // 2, cy0, mid - side // 2 + side, cy0 + side)
            face = sheet.crop(box).resize((SIZE, SIZE), Image.BOX)
            strip.paste(face, (0, (r * cols + c) * SIZE))

    strip.save(dst)
    print("portrait strip %dx%d  (%d frames of %dx%d)"
          % (SIZE, SIZE * cols * rows, cols * rows, SIZE, SIZE), file=sys.stderr)


if __name__ == "__main__":
    main()
