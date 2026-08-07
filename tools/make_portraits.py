#!/usr/bin/env python3
"""Slice a 5x4 mood sheet into a vertical portrait strip.

The sheets are not on an even grid: nominal 5x4 cells put the shoulders of one
row inside the top of the next, so an arithmetic slice bleeds. Row and column
boundaries are found from content density instead.

The crop is then one decision for the whole sheet - a single square size, hair
line and centring, taken as medians across all twenty cells - rather than per
frame. Sizing each frame from its own bounding box lets an anger mark or a
sweat drop resize the face.

All frames share one image so they share one 16-colour palette and one GPU
texture; the runtime picks a source rect by mood index.
"""
import argparse
import sys

import numpy as np
from PIL import Image

SIZE = 48
DARK = 60          # summed-RGB level below which a pixel counts as background


def gaps(density, thresh, min_run=5):
    """Index ranges where the sheet is effectively empty: seams and margins.

    The threshold is never 0 because the seams carry compression noise from
    the source render.
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


def bands_at(density, thresh):
    """Content bands, i.e. everything the gaps leave behind."""
    bands, cursor = [], 0
    for a, b in gaps(density, thresh):
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
    return bands


def spans(density, count, axis):
    """Content bands, with the seam threshold recovered from the sheet.

    How empty a seam is depends on how tightly the artist packed the cells,
    so it is a property of the image rather than a constant. HERO-BEST-MOOD-01
    separates cleanly at 2%; MERCHANT-MOOD-01's hoods almost touch and its
    shallowest seam sits at 2.4%, which a hardcoded 2% merges into one band of
    four faces.

    The search walks upward from the tightest threshold and stops at the first
    value that resolves exactly `count` cells. Smallest-that-works is the
    right end to start from: a threshold larger than necessary begins eating
    hair and shoulders into the seam.
    """
    for step in range(5, 151):
        thresh = step / 1000.0
        bands = bands_at(density, thresh)
        if len(bands) == count:
            print("  %s seam threshold %.3f" % (axis, thresh), file=sys.stderr)
            return bands

    raise SystemExit(
        "  ! no threshold resolves %d %s bands; at 0.02 the sheet gives %s.\n"
        "    The cells are touching - add a few pixels of pure black between "
        "them." % (count, axis, bands_at(density, 0.02)))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("src")
    ap.add_argument("dst")
    # Framing is an art decision, so it gets a knob rather than another
    # heuristic. The measured medians are a good default for a bare head; a
    # character whose silhouette is much wider than his face - JACK's hood
    # runs well left of it - needs the square moved and tightened by eye.
    ap.add_argument("--side", type=int, default=0,
                    help="square size in source pixels (default: measured)")
    ap.add_argument("--shift", default="0,0", metavar="DX,DY",
                    help="nudge the square within each cell")
    a = ap.parse_args()
    src, dst = a.src, a.dst
    shift_x, shift_y = (int(v) for v in a.shift.split(","))
    cols, rows = 5, 4

    sheet = Image.open(src).convert("RGB")
    lum = np.asarray(sheet, int).sum(axis=2)
    solid = lum > DARK

    row_bands = spans(solid.mean(axis=1), rows, "row")
    col_bands = spans(solid.mean(axis=0), cols, "col")

    # The band edges are noisy - they move with the seam threshold and with
    # whatever each frame has drawn near its margin. The cells themselves are
    # a regular grid, so fit one: a robust pitch and origin from all the band
    # starts, and every cell is then placed arithmetically.
    def fit(bands, extent):
        starts = [a for a, _ in bands]
        pitch = float(np.median(np.diff(starts))) if len(starts) > 1 else extent
        origin = float(np.median([a - i * pitch for i, a in enumerate(starts)]))
        return origin, pitch

    ox, px = fit(col_bands, solid.shape[1])
    oy, py = fit(row_bands, solid.shape[0])
    print("  grid  origin (%.0f, %.0f)  pitch (%.1f, %.1f)" % (ox, oy, px, py),
          file=sys.stderr)

    # Pass one: measure every cell against the fitted grid. Nothing is cropped
    # yet, because a frame must not be framed by its own decorations.
    #
    # 1.1 sized each square from that cell's own content bounding box, so a
    # face with anger marks beside it measured 244 px wide against 201 for a
    # plain one and was therefore drawn 21% smaller in the same 48x48 box. And
    # centring on the pixel mass fails the other way round: JACK's hood is a
    # large shape sitting left of his face, so the mass centre is not the head.
    #
    # Every cell holds the same character in the same pose, so the framing is
    # one decision for the sheet - medians of the left, top and width offsets
    # across all twenty cells - and a decoration can no longer move it.
    lefts, tops, widths = [], [], []
    cells = []
    for r in range(rows):
        for c in range(cols):
            x0, y0 = int(ox + c * px), int(oy + r * py)
            x1, y1 = int(ox + (c + 1) * px), int(oy + (r + 1) * py)
            x0, y0 = max(x0, 0), max(y0, 0)
            cell = solid[y0:y1, x0:x1]
            ys, xs = np.nonzero(cell)
            if len(xs) == 0:
                raise SystemExit("  ! cell %d,%d is empty" % (r + 1, c + 1))
            cells.append((x0, y0))
            lefts.append(int(xs.min()))
            tops.append(int(ys.min()))
            widths.append(int(xs.max() - xs.min() + 1))

    side = a.side if a.side else int(np.median(widths))
    left = int(np.median(lefts)) + shift_x
    top = int(np.median(tops)) + shift_y
    print("  frame %dx%d at cell offset (%d, %d)  (widths %d..%d)"
          % (side, side, left, top, min(widths), max(widths)), file=sys.stderr)

    # Pass two: one square per cell, same size, same offset. Shoulders fall
    # off the bottom edge; the face is what a 48x48 portrait is for.
    strip = Image.new("RGB", (SIZE, SIZE * cols * rows))
    for i, (x0, y0) in enumerate(cells):
        box = (x0 + left, y0 + top, x0 + left + side, y0 + top + side)
        face = sheet.crop(box).resize((SIZE, SIZE), Image.BOX)
        strip.paste(face, (0, i * SIZE))

    strip.save(dst)
    print("portrait strip %dx%d  (%d frames of %dx%d)"
          % (SIZE, SIZE * cols * rows, cols * rows, SIZE, SIZE), file=sys.stderr)


if __name__ == "__main__":
    main()
