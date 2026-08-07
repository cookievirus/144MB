#!/usr/bin/env python3
"""check_asset.py - validate source art against the IRON & INVESTMENT pipeline.

Build-time only; nothing here ships. Run it before build_assets.bat so a bad
source PNG fails at the tool instead of showing up as a sliver, a halo or a
mis-sliced portrait on screen.

Usage:
  check_asset.py bg      BG-02-SHOP-A01.png
  check_asset.py char    MERCHANT-01.png
  check_asset.py mood    MERCHANT-MOOD-01.png

Exit code 0 = ship it, 1 = fix it first.
"""

import sys
from pathlib import Path

import numpy as np
from PIL import Image

# --- pipeline contract -----------------------------------------------------
BG_ASPECT = 4.0 / 3.0
BG_MIN_W = 1024
MOOD_COLS, MOOD_ROWS = 5, 4
MOOD_DARK = 60          # must match make_portraits.py
FRINGE_BAND = 2         # px inside the silhouette edge sampled for a matte halo

_fails = []
_warns = []


def bad(msg):
    _fails.append(msg)


def warn(msg):
    _warns.append(msg)


def load(path):
    im = Image.open(path)
    a = np.asarray(im.convert("RGBA"), dtype=np.int32)
    return im, a[:, :, :3], a[:, :, 3]


def uniform_border(rgb):
    """Same detection png2c.py uses, reported as counts per edge."""
    h, w, _ = rgb.shape

    def flat(line):
        return np.ptp(line.reshape(-1, 3), axis=0).max() < 2.0

    l = r = t = b = 0
    while l < w - 1 and flat(rgb[:, l]):
        l += 1
    while r < w - l - 1 and flat(rgb[:, w - 1 - r]):
        r += 1
    while t < h - 1 and flat(rgb[t, :]):
        t += 1
    while b < h - t - 1 and flat(rgb[h - 1 - b, :]):
        b += 1
    return l, r, t, b


# --- backdrop --------------------------------------------------------------
def check_bg(path):
    im, rgb, alpha = load(path)
    w, h = im.size
    print(f"  {w}x{h}  {im.mode}  aspect {w / h:.4f}")

    if abs(w / h - BG_ASPECT) > 0.002:
        bad(f"aspect is {w / h:.4f}, must be 4:3 (1.3333). "
            f"For {h}px tall the width must be {round(h * BG_ASPECT)}.")
    if w < BG_MIN_W:
        warn(f"width {w} is below the {BG_MIN_W} reference; the 320x240 "
             f"downscale will have less detail to average.")

    l, r, t, b = uniform_border(rgb)
    if (l, r, t, b) != (0, 0, 0, 0):
        bad(f"uniform padding: left {l} right {r} top {t} bottom {b} px. "
            f"Crop it, or pass --trim-border to png2c.py.")

    if (alpha < 255).any():
        warn("backdrop carries transparency; it will be flattened onto black.")

    print(f"  unique colours {len(np.unique(rgb.reshape(-1, 3), axis=0)):,}")


# --- character -------------------------------------------------------------
def check_char(path):
    im, rgb, alpha = load(path)
    w, h = im.size
    print(f"  {w}x{h}  {im.mode}")

    # Mode "P" with a tRNS chunk is a perfectly good cutout, so the test is
    # on the converted alpha rather than on the mode string.
    if (alpha == 255).all():
        bad("no usable alpha channel. The silhouette must be cut out; a flat "
            "black or white background is baked into the sprite by png2c.py "
            "unless --key-white is used, and --key-white only keys WHITE.")
        return

    cut = alpha > 127
    if not cut.any():
        bad("image is fully transparent.")
        return

    print(f"  transparent {100 * (alpha == 0).mean():.1f}%  "
          f"opaque {100 * (alpha == 255).mean():.1f}%")

    ys, xs = np.nonzero(cut)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    print(f"  silhouette bbox {x1 - x0}x{y1 - y0} at ({x0},{y0}) "
          f"-> {100 * cut.mean():.1f}% coverage")
    if (y1 - y0) < h * 0.5:
        warn("silhouette fills less than half the canvas height; --crop will "
             "handle it, but the source is mostly empty pixels.")

    # A matte that was flattened before the alpha was cut leaves a rim of
    # near-black or near-white pixels one or two px inside the edge.
    p = np.pad(cut, 1, constant_values=False)
    inner = (p[0:h, 1:w + 1] & p[2:h + 2, 1:w + 1] &
             p[1:h + 1, 0:w] & p[1:h + 1, 2:w + 2])
    edge = cut & ~inner
    if edge.any():
        lum = rgb[edge].mean(axis=1)
        dark = (lum < 40).mean()
        light = (lum > 235).mean()
        print(f"  edge rim: {100 * dark:.1f}% near-black, "
              f"{100 * light:.1f}% near-white")
        if dark > 0.35:
            warn(f"{100 * dark:.0f}% of the rim is near-black. Fine if that is "
                 f"a deliberate outline (HERO-BEST-07 measures the same), a "
                 f"problem if the art was flattened onto black before the "
                 f"alpha was cut - compare against the hero before shipping.")
        elif light > 0.35:
            warn("rim is mostly near-white; use --key-white --white-cut.")

    semi = ((alpha > 0) & (alpha < 255)).mean()
    if semi > 0.06:
        warn(f"{100 * semi:.1f}% of pixels are partially transparent; "
             f"--alpha-cut sets the hard threshold at 320x240.")


# --- mood sheet ------------------------------------------------------------
def _gaps(density, thresh, min_run=5):
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


def _bands_at(density, thresh):
    bands, cursor = [], 0
    for a, b in _gaps(density, thresh):
        if a > cursor:
            bands.append((cursor, a))
        cursor = b
    if cursor < len(density):
        bands.append((cursor, len(density)))
    if bands:
        widest = max(b - a for a, b in bands)
        bands = [(a, b) for a, b in bands if (b - a) * 3 >= widest]
    return bands


def _spans(density, count):
    """Mirrors make_portraits.py: the seam threshold is searched for, not
    assumed, so this reports what the real tool will actually do."""
    for step in range(5, 151):
        thresh = step / 1000.0
        bands = _bands_at(density, thresh)
        if len(bands) == count:
            return bands, thresh
    return _bands_at(density, 0.02), None


def check_mood(path):
    """Runs make_portraits.py's own band detection so a sheet that will fail
    the slicer fails here instead, with the measured bands to fix it by."""
    im, rgb, _ = load(path)
    w, h = im.size
    print(f"  {w}x{h}  {im.mode}")

    lum = rgb.sum(axis=2)
    solid = lum > MOOD_DARK

    bg = lum[~solid]
    print(f"  background {100 * (~solid).mean():.1f}% of the sheet")
    if (~solid).mean() < 0.05:
        bad("almost no background below the dark threshold. The sheet must "
            "sit on pure black with visible seams between cells.")

    rows, rt = _spans(solid.mean(axis=1), MOOD_ROWS)
    cols, ct = _spans(solid.mean(axis=0), MOOD_COLS)
    print(f"  rows {len(rows)} at threshold {rt}: {rows}")
    print(f"  cols {len(cols)} at threshold {ct}: {cols}")

    if rt is None:
        bad(f"no seam threshold resolves {MOOD_ROWS} rows; the cells are "
            f"touching. Add a few pixels of pure black between them.")
    if ct is None:
        bad(f"no seam threshold resolves {MOOD_COLS} columns; the cells are "
            f"touching. Add a few pixels of pure black between them.")

    if len(rows) == MOOD_ROWS and len(cols) == MOOD_COLS:
        widths = [b - a for a, b in cols]
        heights = [b - a for a, b in rows]
        print(f"  cell widths  {widths}  (spread {max(widths) - min(widths)}px)")
        print(f"  cell heights {heights}  (spread {max(heights) - min(heights)}px)")
        if max(widths) > 1.35 * min(widths):
            warn("column widths vary by more than 35%; the square crop is "
                 "driven by width, so faces will land at different scales.")


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("bg", "char", "mood"):
        print(__doc__)
        return 2

    kind, path = sys.argv[1], Path(sys.argv[2])
    print(f"{path.name}  [{kind}]")
    {"bg": check_bg, "char": check_char, "mood": check_mood}[kind](path)

    for m in _warns:
        print(f"  ~ WARN  {m}")
    for m in _fails:
        print(f"  ! FAIL  {m}")
    print("  OK" if not _fails else "  REJECTED")
    return 1 if _fails else 0


if __name__ == "__main__":
    sys.exit(main())
