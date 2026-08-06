# IRON & INVESTMENT — Demo 1.1

Static forge backdrop with the hero entering from the right on an eased slide.
No VFX, no audio, no gameplay — this is the render + presentation spine that
every later scene will sit on.

**1.1** adds the quality-tier system: every item, gear piece and fitted tool
carries one of eight tiers, shown as an 8×8 ball and a name colour throughout
the menu.

## Build

```bat
build_assets.bat    :: only when the source PNGs change
build.bat
```

Set `RAYLIB` in `build.bat` to your static raylib tree. On Linux/macOS use
`make RAYLIB=/usr/local`.

Controls: **M** command menu · **arrows** move cursor · **SPACE** / **ENTER**
select and advance · **ESC** back one level · **R** replay · **F11** fullscreen.

`ESC` is a back key and nothing else. It closes one layer at a time, and once
nothing is open it raises the system prompt — **CANCEL / SAVE / QUIT**, with
`CANCEL` selected by default. The process only ends on a confirmed `QUIT`, so
a reflexive `ESC` can never throw away a session.

Accept is one action bound to `SPACE`, `ENTER` and numpad enter, resolved in a
single `AcceptPressed()` in `main.c`. Testing each key at its call site is how
"confirm" ends up meaning different things in the dialogue and the menu.

## Size accounting

| Item | Bytes | Note |
|---|---:|---|
| `bg_smithy` payload | 18,087 | 320×240, 32 colours, raw DEFLATE (23.6% of raw) |
| `bg_smithy` palette | 96 | |
| `hero_idle` payload | 3,004 | 78×184, 16 colours + colour key (20.9% of raw) |
| `hero_idle` palette | 48 | |
| `portraits` payload | 9,801 | 48×960 strip, 20 moods, one shared palette |
| `portraits` palette | 48 | |
| `font5x7` | 665 | 95 glyphs, 1 byte per glyph row |
| `rarity` tables | 136 | 8 tiers: mask, colours, multipliers, names |
| **Asset total** | **31,885** | **2.16% of the 1,474,560 budget** |

The dialogue balloon itself costs nothing: panel, name plate, border and
continue caret are all drawn from rectangles. A baked nine-slice panel would
be ~288 bytes and a baked full panel ~12 KB, for a look that would be no
better at a one-pixel corner radius.

Uncompressed those two images would be 89,824 bytes of indices, or 359,296
bytes as raw RGBA. The DEFLATE step is what keeps a full-screen painted
backdrop affordable at all.

Backdrop cost is driven by texture noise, not by resolution. Same source,
same 32-colour palette, only the pre-downscale median filter varies:

| `--denoise` | Bytes | |
|---:|---:|---|
| 0 | 20,777 | every shelf item legible, noisiest |
| 3 | 19,117 | |
| **5** | **18,087** | **shipped — tongs and shelf props still read** |
| 7 | 14,806 | flatter, wall props start dissolving |
| 9 | 12,751 | too soft |

Flat, hand-cleaned pixel art compresses far better than a reconstructed
render. Tidying art in Aseprite is a binary-size optimisation, not just a
cosmetic one.

## Why DEFLATE and not PNG

An optimised indexed PNG of the same backdrop is 16,746 bytes versus 16,564
for a raw DEFLATE stream of the palette indices — a 182-byte difference, so
the payloads are effectively a wash. The decision is made at the *decoder*:

- **PNG** would need `SUPPORT_FILEFORMAT_PNG` and therefore `stb_image`
  linked into the binary.
- **Raw DEFLATE** reuses `DecompressData()`, which is already compiled in via
  `SUPPORT_COMPRESSION_API` (sinfl, roughly 2–3 KB of code).

So the second path lets every image decoder be switched off in `config.h`.
I have not measured the exact `stb_image` saving on this toolchain yet — that
is on the open-questions list — but it is tens of kilobytes, not hundreds of
bytes, which is why the pipeline is built this way from the start.

Compatibility is verified, not assumed: raylib's `sinflate()` was compiled
against the generated `hero_idle` blob and returned 16,800 bytes with a
checksum identical to Python's `zlib` reference decode.

## Source-art requirements

| Asset class | Source canvas | Result |
|---|---|---|
| Full-screen backdrop | 1024×768 RGB/RGBA | 320×240 |
| Character (hero and NPC) | 600×1024 RGBA | height set per character, see below |

Both must be **exactly 4:3 for backdrops** and **fully transparent outside the
silhouette for characters** — no white matte, no padding strips.

`png2c.py` now detects solid-colour padding on any edge and warns; pass
`--trim-border` to strip it. This exists because the first backdrop was a
1269×952 image padded to 1288 with a 19 px white column, which survived the
downscale as a white sliver down the right edge of the screen. That class of
bug is now caught by the tool instead of by eye.

1024 → 320 is a 3.2× reduction rather than a whole number. That would matter
if the source had a real pixel grid to stay in phase with; it does not (see
below), so it costs nothing. 1280×960 would only be worth switching to if the
backdrops are ever authored as true pixel art.

### Character heights

Canvas size does not set character scale — `--crop` discards the canvas and
`--h` sets the height. So every character can share the 600×1024 canvas while
still differing in height, which is what makes NPC scale controllable:

| Role | Height | Bytes |
|---|---:|---:|
| Adult male (hero) | **176** | 2,797 |
| Adult female | 168 | ~2,600 |
| Child | 120 | ~1,400 |
| Large NPC (knight, ogre) | 190 | ~3,200 |

### Framing beats scale

The hero ships at 184, not the 176 that a full-body shot tolerates. Full-body
framing has a hard ceiling: at 192 his hip clears the workbench and his head
overlaps the tool shelf, so he stops reading as being *in* the room.

Cutting the legs removes that ceiling. Only the top 167 rows are on screen and
the rest falls past y=240, which the viewer reads as "close to camera" rather
than "too big for the room". The scale that looked wrong standing on the floor
looks right cut at the shin.

Measured off the concept frame rather than eyeballed: the hero occupies
x 766..1016, y 234..768 of the 1024×768 concept, which is 78×167 at 320×240
scale, implying a 184 full-body height. The generated sprite came out 78×184.

### Do not animate at this size

One 80×176 frame costs roughly 2.8 KB. A six-pose set at four frames each
would be 67 KB for one character. Split body and head instead:

| Layer | Size | Cost |
|---|---|---:|
| Body pose | 80×176 | ~2,800 B each |
| Head / expression overlay | 48×56 | ~540 B each |

Twenty expressions then cost ~11 KB rather than ~56 KB, and any head composes
with any body pose without redrawing either.

### Neither source has a pixel grid

Downsampling by *N* and re-expanding gives a smoothly rising error for every
*N* from 2 to 12 — no dip anywhere. True pixel art upscaled by *N* would show
a sharp minimum at *N*. Horizontal run lengths in the character art tell the
same story: 97.7% of runs are a single pixel.

These are smooth renders painted in a pixel-art style, so every reduction is a
reconstruction. Good enough to build on; hand-cleaning at 320×240 before
submission is still the step that turns them into actual pixel art.

### One known wart

raylib's `DecompressData()` allocates a fixed 64 MB scratch buffer before
inflating, then reallocs down. It is transient and boot-only, but it is ugly.
Options if it becomes a problem: call `sinflate()` directly against a
correctly sized buffer (`raw_size` is already stored in `EmbeddedImage`), or
patch the constant in a vendored `rutils.c`. Deferred to the Week 7 audit.

## Suggested `config.h` for this demo

```c
#define SUPPORT_COMPRESSION_API     1   /* required by GfxLoadTexture */

/* Every image decoder off — nothing loads from disk. */
//#define SUPPORT_FILEFORMAT_PNG
//#define SUPPORT_FILEFORMAT_BMP
//#define SUPPORT_FILEFORMAT_QOI
//#define SUPPORT_FILEFORMAT_DDS

/* Modules the demo never touches. */
//#define SUPPORT_MODULE_RMODELS
//#define SUPPORT_MODULE_RAUDIO       /* re-enable in Week 5 */
//#define SUPPORT_GESTURES_SYSTEM
//#define SUPPORT_CAMERA_SYSTEM
//#define SUPPORT_DEFAULT_FONT        /* re-enable when UI text lands */
```

`SUPPORT_DEFAULT_FONT` is worth a before/after measurement on its own — the
embedded atlas is not small, and the game will eventually want a custom
bitmap font anyway.

## Asset pipeline

`tools/png2c.py` is build-time only; nothing in it ships.

```
png2c.py <in.png> <out.h> <symbol> [options]

  --w N --h N        target size; give one and the other is derived
  --colors N         palette size (includes the key slot when --key-white)
  --key-white        derive alpha from a white matte (RGB sources only)
  --crop             autocrop to the visible bounding box
  --denoise N        median filter before downscale
  --white-cut N      brightness above which a near-grey pixel is background
  --alpha-cut F      coverage threshold for the hard alpha edge
  --trim-border      strip solid-colour padding strips (warns if unset)
```

The colour key is decided by the image, not by the flags: if any pixel survives
as transparent, palette slot 0 is reserved for it. An RGBA source needs no
flag at all.

Two details that matter for these particular reference renders:

**Alpha-weighted downscale.** The hero is anti-aliased against white, so a
plain resize drags white into the silhouette and leaves a bright fringe.
Colour is accumulated weighted by coverage and divided by coverage, then
alpha is hard-thresholded so no anti-aliased pixel survives into a 320×240
target.

**Desaturation-aware key.** Keying on `rgb > 240` still left a grey halo,
because the fringe pixels sit around 215–240. The key now also requires the
pixel to be near-grey (`max − min < 24`), which drops the fringe while
keeping the cream shirt, whose blue channel is far below its red.

Both source images are upscaled concept renders, not grid-aligned pixel art,
so the pipeline is doing real reconstruction rather than a straight import.
The output is good enough to ship a demo on and worth redrawing by hand
before final submission.

## Dialogue

`SPACE` or `ENTER` advances. The first press completes the line if it is still
revealing; the second moves to the next line. That ordering matters: a player
reading faster than the reveal wants the rest of the sentence, not a skip.

Reveal mode is per-dialogue, on `UiDialog.reveal`:

| Mode | Step | Behaviour |
|---|---:|---|
| `REVEAL_WORD` (default) | 105 ms | snaps to the next space, whole words only |
| `REVEAL_CHAR` | 30 ms | classic typewriter |

Both drive the same `shown` character counter, so word mode is a different
step function rather than a second code path.

Lines are hand-wrapped with `\n`. The balloon holds **39 columns × 3 rows**.
An automatic wrapper is deliberately absent: the script is authored by hand,
so wrapping is a writing decision, and the code would cost bytes to re-derive
something the author already knows.

### Portraits

Twenty expressions live in one 48×960 vertical strip: one palette, one
texture, one bind, and the mood index picks a source rect. Twenty separate
images would cost twenty palettes and twenty texture binds for no visual gain,
since every frame is the same face under the same light.

`tools/make_portraits.py` does **not** slice on an arithmetic grid. The source
sheet's rows touch — the shoulders of one row sit inside the top of the next —
so an even 5×4 split bleeds the previous row into every frame. Row and column
boundaries are recovered from content density instead, then each face is
squared off anchored to the top of its hair.

### Font

`tools/font5x7.py` holds 95 hand-placed glyphs, 5×7 in a 6×8 cell, emitted as
665 bytes with one byte per glyph row. Hand-authored rather than derived from
a TTF because at five-pixel cap height, hinting a vector face produces mush.

At boot the bit table is expanded into one white glyph atlas, so drawing a
string is one `DrawTextureRec` per character and colour is a tint. Blitting
set bits directly would be up to 35 one-pixel rectangles per glyph.

This is also what lets `SUPPORT_DEFAULT_FONT` stay off in `config.h`.

**ASCII only.** Thai needs above/below vowel and tone-mark stacking, which is
a shaping problem rather than a glyph-count problem. If the game ever needs
Thai text, that is a separate piece of work, not a bigger font table.

## Layers

Three layers, and input only ever reaches the topmost one:

| Layer | Modal | `ESC` |
|---|---|---|
| System prompt | yes — dims the frame | closes it |
| Command menu | no | pops one screen |
| Dialogue balloon | no | hides it |
| *(nothing open)* | — | raises the system prompt |

`UiPrompt` is written generically — a title, up to four options, an optional
note — rather than as a bespoke quit box, because every confirmation the game
will want later is the same shape: selling a blade, travelling, abandoning a
commission. A one-off quit dialog would be identical code with the labels
welded in.

Only the prompt washes the frame. Modal and non-modal must not look alike, or
the player cannot tell what a key press will reach.

`SAVE` writes a versioned blob (magic, version, phase, script index) through
raylib's `SaveFileData`. There is little state worth keeping in a demo; the
point is that the plumbing and the version byte exist before there is
anything at stake. The result is reported back in the prompt itself rather
than as a toast, since that is where the player is looking.

## Command menu

`M` opens it. Arrows move the cursor, `ENTER` descends, `ESC` backs out one
level and closes the menu once nothing is left.

```
ROOT ─┬─ TALK       closes the menu, plays a solo script
      ├─ INVENTORY  left/right switch category, up/down pick an item
      ├─ EQUIPMENT  2x3 slot grid, all four arrows
      └─ MAP        destination list (travel not implemented)
```

Navigation is a **stack of frames**, not hand-written transitions. `ESC` pops
exactly one frame, always; popping the last one closes the menu. Adding a
screen therefore cannot break the back behaviour, which is the failure mode
that bites every menu written as a flat state machine.

Only the top frame is drawn. Stacking translucent panels would dim the scene
twice over and make the alpha read as a bug.

`TALK` returns a `MENU_CMD_TALK` command rather than playing the script
itself: the menu draws and navigates, the scene owns world state. The menu
never touches `UiDialog`.

### Content tables

`src/game_data.h` holds items, equipment slots and destinations as `const`
tables in `.rodata`. Items live in **one flat array** filtered by category on
each key press. Per-category arrays would cost pointer tables in `.rodata`
permanently to save a few dozen comparisons on a key press that a human just
made.

Descriptions are hand-wrapped to the 23-column detail pane, matching the
dialogue decision. That stops paying off at roughly fifty entries, at which
point a word-wrap routine earns its ~150 bytes.

**No item icons yet.** At 16x16 and 16 colours they run about 100 bytes each
after DEFLATE, so a full set is affordable — it is art that does not exist,
not a budget problem.

## Quality tiers

Eight tiers, ordered `JUNK → COMMON → UNCOMMON → RARE → EPIC → LEGENDARY →
MYTH`, with `CURSED` last. Cursed is *not* stronger than Myth; it is the most
profitable and the most dangerous, which is a different axis. It sits at the
end of the enum because a linear ladder is cheaper than a tier plus a flag,
and nothing yet needs a Cursed Legendary. When something does, lift it out
into a one-bit flag on `ItemDef` — about twenty bytes of code and no new data.

Nothing here is a sprite. One 8×8 shape mask at 2 bits per pixel and one base
colour per tier are baked into a 64×8 strip at boot; rim and highlight are
derived from the base, so retuning the palette is a two-constant edit rather
than an art pass.

| | Bytes | |
|---|---:|---|
| `RARITY_MASK` | 16 | 8 rows × 2 bpp |
| `RARITY_BASE` | 32 | one hue per tier |
| `RARITY_MUL` | 8 | price multiplier, eighths |
| `RARITY_LABEL` | 80 | fixed 10-char stride |
| code (`-O2`, measured) | 976 | `RarityLoad` alone is 405 |
| **total** | **1,132** | 0.08% of the budget |

Embedding eight finished sprites instead would be roughly 600 bytes cheaper.
The trade is that every colour change would then need a regenerated asset
header, and the two collisions below were both found *after* the palette was
first written.

### Two collisions, both fixed by hue rather than by value

**Junk vs Common.** The conventional pairing is grey and white. At eight
pixels with a white specular highlight on both, they read as the same object.
Junk is now rust `0x7A6248` and Common is clean steel `0xDCE0E8` — different
in hue, value and temperature. It also says the right thing: broken gear in a
smithy is rusted, not grey.

**Cursed vs Common.** `RarityTint` lifts near-black tiers so they survive
against `UI_SHADE`. Blending toward white did that correctly and landed
Cursed on the same off-white as Common. The lift now scales all three
channels by one factor until the brightest reaches 200, which raises the
value without touching the hue: Cursed reads violet at `(163, 136, 200)`.

The luma threshold is 80, not 72. At 72 Cursed sat three points under the
line and any future nudge to its base colour would have silently flipped it
to the unlifted branch. Myth, the next darkest tier, is at 97.

### Names are tinted on every row, not only the selected one

Dimming unselected rows is the usual way to show focus, and it would drain
the colour out of eight rows to mark one. Scanning a list at a glance is the
entire point of the ball, so focus is left to the selection bar and the caret,
which do not compete with hue.

### Value scaling

`RarityScaleValue` is fixed point in eighths — `x0.25` for Junk through `x12`
for Myth, with Cursed at `x3`. No floats and no libm. It is unused until the
economy lands in Week 4; drop it for 36 bytes if the Week 7 audit is tight.


## Architecture

```
src/main.c             window, backbuffer, loop       (unity build root)
src/gfx.{h,c}          EmbeddedImage decode, pillarbox
src/rarity.{h,c}       quality tiers: ball strip, tints, value scaling
src/ui.{h,c}           notched panel, shared palette
src/ui_font.{h,c}      glyph atlas, text drawing
src/ui_dialog.{h,c}    balloon, portrait, reveal state machine
src/ui_menu.{h,c}      command window, screen stack
src/game_data.h        item / equipment / destination tables
src/scene_smithy.{h,c} demo scene state machine
assets/*.h             generated, do not hand-edit
```

Everything draws into a 320×240 `RenderTexture2D` and is upscaled once at
present time. `GFX_INTEGER_SCALE` snaps that upscale to whole multiples so
source pixels stay square; the cost is slightly thicker letterbox bars.

The entrance is a three-state machine (`HOLD → ENTER → SETTLED`) rather than a
free-running timer, so later scenes can hang triggers off phase transitions
without re-deriving them from elapsed seconds.

Hero X is snapped to whole virtual pixels at draw time. Sub-pixel positioning
under point filtering makes the sprite shimmer against the backdrop's pixel
grid; the trade is that motion quantises to 1-pixel steps, which is the
correct look for the target aesthetic.

## Test checklist

1. Hero starts fully off-screen — nothing visible at the right edge on frame 1.
2. Motion decelerates into rest; it should not stop abruptly.
3. **R** replays cleanly from any phase.
4. **F11** and window resize keep 4:3 with black bars, no stretching.
5. Drag the window during the entrance — `DT_CLAMP` should stop the hero from
   teleporting.
6. Rest position: hero left edge at x=222, 24 px of margin on the right.
7. Backdrop bleeds to all four screen edges — no light sliver at the right.

## Next

- `HERO_REST_X` / `HERO_REST_Y` / `ENTER_SECONDS` are the tuning knobs.
- A 1–2 px vertical bob during `SMITHY_ENTER` would sell the walk without a
  real walk cycle. Deliberately left out of 1.0.
- Forge glow and lantern flicker belong in a VFX pass driven off a shared
  scene clock, not baked into `scene_smithy.c`.
