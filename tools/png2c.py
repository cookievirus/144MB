#!/usr/bin/env python3
"""
png2c.py - IRON & INVESTMENT asset pipeline (build-time only).

Converts a PNG into a C header holding:
  - an RGB palette (3 bytes per entry)
  - a raw-DEFLATE stream of 8-bit palette indices

Runtime cost: raylib's DecompressData() (sinfl) + a palette expand loop.
No PNG decoder needed at runtime, so SUPPORT_FILEFORMAT_* can be stripped
from raylib's config.h.

Usage:
  png2c.py in.png out.h SYMBOL --w 320 --h 240 --colors 32
  png2c.py in.png out.h SYMBOL --h 200 --colors 16 --key-white
"""

import argparse
import zlib
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

def load_rgba(path: Path, key_white: bool, white_cut: int):
    """Return (rgb float array HxWx3, alpha float array HxW in 0..1)."""
    im = Image.open(path)

    # Palette PNGs carry transparency in a tRNS chunk, not in a channel, and
    # an indexed export from Aseprite is mode "P". Testing the mode string
    # alone silently dropped that alpha and baked the background into the
    # sprite as a solid block. Normalise anything that has transparency at
    # all before looking at channels.
    if im.mode in ("RGBA", "LA", "PA") or "transparency" in im.info:
        im = im.convert("RGBA")

    if im.mode == "RGBA":
        a = np.asarray(im, dtype=np.float64)
        return a[:, :, :3], a[:, :, 3] / 255.0

    rgb = np.asarray(im.convert("RGB"), dtype=np.float64)
    if not key_white:
        return rgb, np.ones(rgb.shape[:2])

    # The reference render is anti-aliased against white, so a plain
    # "== 255" key leaves a bright fringe. Anything bright AND near-grey is
    # treated as background; the cream shirt stays because its blue channel
    # is far below its red channel.
    lo = rgb.min(axis=2)
    hi = rgb.max(axis=2)
    background = (lo > white_cut) & ((hi - lo) < 24)
    return rgb, (~background).astype(np.float64)


def trim_uniform_border(rgb, alpha, warn_only):
    """Strip solid-colour padding strips from the edges.

    Generators sometimes emit a 4:3 image padded out to an odd canvas width.
    That padding survives the downscale as a bright sliver at the screen edge,
    so it is detected here rather than left for the eye to catch.
    """
    h, w = alpha.shape
    l, r, t, b = 0, w, 0, h

    def uniform(line):
        return np.ptp(line.reshape(-1, 3), axis=0).max() < 2.0

    while l < r - 1 and uniform(rgb[:, l]):
        l += 1
    while r > l + 1 and uniform(rgb[:, r - 1]):
        r -= 1
    while t < b - 1 and uniform(rgb[t, :]):
        t += 1
    while b > t + 1 and uniform(rgb[b - 1, :]):
        b -= 1

    if (l, r, t, b) == (0, w, 0, h):
        return rgb, alpha

    print(
        "  ! uniform border detected: left %d right %d top %d bottom %d px%s"
        % (l, w - r, t, h - b, "" if not warn_only else "  (use --trim-border)"),
        file=sys.stderr,
    )
    if warn_only:
        return rgb, alpha
    return rgb[t:b, l:r], alpha[t:b, l:r]


def autocrop(rgb, alpha, thresh=0.06):
    """Ignore the faint alpha halo so the bbox matches the real silhouette."""
    ys, xs = np.nonzero(alpha > thresh)
    if len(xs) == 0:
        return rgb, alpha
    y0, y1 = ys.min(), ys.max() + 1
    x0, x1 = xs.min(), xs.max() + 1
    return rgb[y0:y1, x0:x1], alpha[y0:y1, x0:x1]


def resize_premultiplied(rgb, alpha, dw, dh):
    """Alpha-weighted box downsample.

    Plain resizing bleeds the keyed-out background into the sprite edge and
    leaves a white halo. Weighting colour by coverage avoids that; the alpha
    channel is then hard-thresholded so no anti-aliased pixels survive.
    """
    sh, sw = alpha.shape
    prem = rgb * alpha[:, :, None]

    def box(chan):
        img = Image.fromarray(chan.astype(np.float32), mode="F")
        return np.asarray(img.resize((dw, dh), Image.BOX), dtype=np.float64)

    acc = np.dstack([box(prem[:, :, c]) for c in range(3)])
    cov = box(alpha)
    safe = np.maximum(cov, 1e-6)
    out_rgb = np.clip(acc / safe[:, :, None], 0, 255)
    return out_rgb, cov


def quantize(rgb, mask, ncolors):
    """Median/max-coverage quantize only the visible pixels."""
    h, w = mask.shape
    flat = rgb.reshape(-1, 3)
    vis = mask.reshape(-1)

    # Keyed-out pixels are pulled to the mean visible colour so they cannot
    # claim a palette slot of their own.
    if vis.any() and not vis.all():
        flat = flat.copy()
        flat[~vis] = flat[vis].mean(axis=0)

    src = Image.fromarray(flat.reshape(h, w, 3).astype(np.uint8), "RGB")
    q = src.quantize(colors=ncolors, method=Image.MAXCOVERAGE, dither=Image.NONE)
    idx = np.asarray(q, dtype=np.uint8)
    pal = np.asarray(q.getpalette()[: ncolors * 3], dtype=np.uint8).reshape(-1, 3)
    return idx, pal


def reserve_transparent(idx, pal, mask):
    """Shift every index up by one so slot 0 is the colour key."""
    idx = idx.astype(np.uint16) + 1
    idx[~mask] = 0
    pal = np.vstack([np.zeros((1, 3), np.uint8), pal])
    return idx.astype(np.uint8), pal


def deflate(raw: bytes) -> bytes:
    c = zlib.compressobj(9, zlib.DEFLATED, -15)  # raw stream, sinfl-compatible
    return c.compress(raw) + c.flush()


def emit(sym, w, h, pal, blob, raw_len, keyed, out: Path):
    def rows(data, per_line):
        for i in range(0, len(data), per_line):
            yield ",".join("%d" % b for b in data[i : i + per_line])

    guard = "ASSET_%s_H" % sym.upper()
    L = []
    L.append("/* Generated by tools/png2c.py - do not edit by hand. */")
    L.append("#ifndef %s" % guard)
    L.append("#define %s" % guard)
    L.append("")
    L.append('#include "gfx.h"')
    L.append("")
    L.append("static const unsigned char %s_pal[%d] = {" % (sym, pal.size))
    for r in rows(pal.reshape(-1), 24):
        L.append("    %s," % r)
    L.append("};")
    L.append("")
    L.append("static const unsigned char %s_blob[%d] = {" % (sym, len(blob)))
    for r in rows(blob, 24):
        L.append("    %s," % r)
    L.append("};")
    L.append("")
    L.append("static const EmbeddedImage %s = {" % sym)
    L.append("    %d, %d, %d, %d," % (w, h, len(pal), 1 if keyed else 0))
    L.append("    %s_pal, %s_blob, %d, %d" % (sym, sym, len(blob), raw_len))
    L.append("};")
    L.append("")
    L.append("#endif /* %s */" % guard)
    out.write_text("\n".join(L) + "\n", encoding="ascii")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("src")
    p.add_argument("dst")
    p.add_argument("symbol")
    p.add_argument("--w", type=int, default=0, help="target width (0 = derive)")
    p.add_argument("--h", type=int, default=0, help="target height (0 = derive)")
    p.add_argument("--colors", type=int, default=16)
    p.add_argument("--key-white", action="store_true", help="treat white as alpha")
    p.add_argument("--crop", action="store_true", help="autocrop to visible bbox")
    p.add_argument("--denoise", type=int, default=0, help="median filter radius")
    p.add_argument("--alpha-cut", type=float, default=0.5)
    p.add_argument("--white-cut", type=int, default=210)
    p.add_argument("--trim-border", action="store_true",
                   help="strip solid-colour padding strips (warn only if unset)")
    a = p.parse_args()

    src = Path(a.src)
    rgb, alpha = load_rgba(src, a.key_white, a.white_cut)
    if not a.crop:  # --crop already discards padding via the alpha bbox
        rgb, alpha = trim_uniform_border(rgb, alpha, warn_only=not a.trim_border)

    if a.denoise:
        img = Image.fromarray(rgb.astype(np.uint8), "RGB")
        rgb = np.asarray(img.filter(ImageFilter.MedianFilter(a.denoise)), np.float64)

    if a.crop:
        rgb, alpha = autocrop(rgb, alpha)

    sh, sw = alpha.shape
    dw, dh = a.w, a.h
    if dw and not dh:
        dh = max(1, round(sh * dw / sw))
    elif dh and not dw:
        dw = max(1, round(sw * dh / sh))
    elif not dw and not dh:
        dw, dh = sw, sh

    rgb, cov = resize_premultiplied(rgb, alpha, dw, dh)
    mask = cov >= a.alpha_cut

    # Transparency is a property of the image, not of how alpha was derived:
    # any keyed-out pixel means slot 0 must be reserved for the colour key.
    keyed = not mask.all()
    idx, pal = quantize(rgb, mask, a.colors - 1 if keyed else a.colors)
    if keyed:
        idx, pal = reserve_transparent(idx, pal, mask)

    raw = idx.tobytes()
    blob = deflate(raw)
    emit(a.symbol, dw, dh, pal, blob, len(raw), keyed, Path(a.dst))

    ratio = 100.0 * len(blob) / len(raw)
    print(
        "%-14s %3dx%-3d  %2d colours  raw %6d B -> deflate %6d B  (%.1f%%)"
        % (a.symbol, dw, dh, len(pal), len(raw), len(blob), ratio),
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
