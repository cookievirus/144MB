# IRON & INVESTMENT — Demo 1.4

Two rooms, travel between them, and a trader who buys and sells. The forge
still lights and the hero still walks on; **M → MAP → Market Row** now takes
him somewhere.

**1.4** puts a front door on the game and makes the shop a conversation:

- **Title screen** — START / LOAD / OPTION / EXIT, with a placeholder backdrop
  that costs no payload at all.
- **LOAD works.** The save blob written since 1.2 is now readable: gold, held
  counts and the room all come back.
- **The counter is JACK's, not a menu's.** The welcome leads straight into it,
  ESC hands the question to him, and "anything else?" resumes exactly where
  you left off.
- **Prices carry their unit** — `POTION 33 G`, not `POTION 33`.
- **The continue caret names its key.** A blinking chevron says there is more;
  it does not say how to get it.

**1.3** added list sorting, and tightened the shop:

- **Sorting.** `R` rarity, `T` quantity, `A` name — on the inventory screen and
  both shop tabs. Pressing the active key again flips the direction.
- **Leaving the counter is confirmed.** ESC at the shop asks instead of acting.
- **Talking to JACK opens his counter.** `M → TALK` plays a line that is *not*
  the welcome, then the shop opens.
- **JACK arrives sooner.** 1.30 s instead of 2.95 s; the whole entrance is now
  2.45 s instead of 4.10 s.
- **Headless tests.** `make test` — three suites under ASan + UBSan.
- **Text can no longer run off a panel.** Hint lines are budgeted at compile
  time; titles, tags, purse and prompt buttons are measured in the tests.

**1.2** turned the spine into a game loop:

- **Rooms are data.** `scene_smithy.{h,c}` is gone. One `SceneDef` row in
  `scene.c` describes a room — backdrop, standing actor, entrance direction,
  scripts, whether it has a counter — and the state machine exists once.
  Room three costs a table row, not a file.
- **An item shop.** Two tabs over one item table, live prices driven by the
  quality tier, per-visit stock, and a purse.
- **Real inventory.** `ITEMS[].qty` was `const`; held counts now live in
  `inventory.c` and the inventory screen reads them, so buying a whetstone
  shows up under **M → INVENTORY**.
- **Travel.** The MAP screen's "TRAVEL NOT IMPLEMENTED" line is gone.
- **JACK.** The shop has a shopkeeper who walks on 2.5 s after BEST does,
  from the door marked PRIVATE, with his own twenty-expression portrait strip.
- **The economy uses the tier ladder.** `RarityScaleValue` shipped unused in
  1.1 with a note offering to delete it for 36 bytes. It now sets every price
  in the game, so retuning a whole tier is one byte in `RARITY_MUL`.

## Build

```bat
build_assets.bat    :: only when the source PNGs change
build.bat
```

Set `RAYLIB` in `build.bat` to your static raylib tree. On Linux/macOS use
`make RAYLIB=/usr/local`.

The game opens on the title screen. **Arrows** move, **SPACE** selects.

In play: **M** command menu · **arrows** move cursor · **SPACE** / **ENTER**
select and advance · **ESC** back one level · **F5** replay · **F11**
fullscreen.

In the shop: **left/right** switch BUY and SELL, **up/down** pick a row,
**SPACE** trades one unit, **ESC** asks whether you are done.

On the inventory screen and in the shop: **R** sort by rarity, **T** by
quantity, **A** by name. The same key again reverses the order.

Replay moved from **R** to **F5** in 1.3. A key that means "reorder this list"
on one screen and "throw the session away" on another is exactly the ambiguity
the input layer is meant to keep out.

`ESC` is a back key and nothing else. It closes one layer at a time, and once
nothing is open it raises the system prompt — **CANCEL / SAVE / QUIT**, with
`CANCEL` selected by default. The process only ends on a confirmed `QUIT`, so
a reflexive `ESC` can never throw away a session.

Accept is one action bound to `SPACE`, `ENTER` and numpad enter, resolved in a
single `AcceptPressed()` in `main.c`. Testing each key at its call site is how
"confirm" ends up meaning different things in the dialogue and the menu.

## Size accounting

Measured with `nm --size-sort -S` on an `-O2` object, not estimated.

| Item | Bytes | Note |
|---|---:|---|
| `bg_smithy` payload | 18,087 | 320×240, 32 colours, raw DEFLATE (23.6% of raw) |
| `bg_smithy` palette | 96 | |
| `bg_shop` payload | 18,040 | 320×240, **48** colours, raw DEFLATE (23.5% of raw) |
| `bg_shop` palette | 144 | |
| `hero_idle` payload | 3,004 | 78×184, 16 colours + colour key (20.9% of raw) |
| `hero_idle` palette | 48 | |
| `merchant_idle` payload | 3,043 | 98×168, JACK (18.5% of raw) |
| `merchant_idle` palette | 48 | |
| `portraits` payload | 8,011 | 48×960 strip, 20 moods, one shared palette |
| `portraits` palette | 48 | |
| `portraits_merchant` payload | 9,795 | 48×960, JACK's 20 moods |
| `portraits_merchant` palette | 48 | |
| `font5x7` | 665 | 95 glyphs, 1 byte per glyph row |
| **Asset subtotal** | **61,077** | |
| code, tables and strings | 22,139 | `.text` + `.rodata` less the assets above |
| `.data` | 1,720 | |
| `.bss` | 116 | 52 of it the live inventory |
| **Object total** | **89,823** | **6.09% of the 1,474,560 budget** |

`portraits` fell from 9,801 to 8,011 without losing a frame — see the framing
fix below. JACK's strip is dearer than BEST's at 9,795 because a hood full of
woven texture compresses worse than hair against black.

Not the binary: static raylib is still the dominant term and is only measured
after linking. This is the part of the budget the project actually authors.

### Why the shop backdrop gets 48 colours

The smithy is one lit forge against dark walls and quantises cleanly at 32.
The shop is a wall of small goods under a warm lamp, and 32 colours spend too
many slots on the doorway's sky and crush every shelf into flat brown. The
props *are* the content in a room the player came to shop in, so the extra
2,561 bytes buy legibility rather than polish.

| `--colors` | `--denoise` | Bytes | |
|---:|---:|---:|---|
| 32 | 5 | 12,412 | shelf goods dissolve, shadows crush |
| 32 | 0 | 15,479 | props return, shadows still blocky |
| **48** | **0** | **18,040** | **shipped — same cost as `bg_smithy`** |
| 64 | 0 | 18,997 | no visible gain over 48 |

Note the inversion against 1.1's finding: for this image the median filter
costs more quality per byte than the palette does. Denoise helps a noisy
*render*; this source is flatter, so filtering only eats detail.

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
src/ui.{h,c}           panel, palette, page chrome, row, detail, number
src/ui_font.{h,c}      glyph atlas, text drawing
src/ui_dialog.{h,c}    balloon, portrait sets, reveal state machine
src/ui_menu.{h,c}      command window, screen stack, travel
src/sort.{h,c}         list ordering shared by every item screen
src/title.{h,c}        title screen: START / LOAD / OPTION / EXIT
src/ui_prompt.{h,c}    modal confirmation
src/inventory.{h,c}    held counts and gold — the only mutable player state
src/shop.{h,c}         BUY / SELL counter
src/game_data.h        item / equipment / destination / stock tables
src/scene.{h,c}        SceneDef table and one room state machine
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

## What 1.2 changed structurally

### One scene, many rooms

Adding the shop as `scene_shop.{h,c}` would have duplicated the entrance
machine, the dialogue pump, the ESC ladder and the save path, and a third room
would have made that three copies. Both rooms are the same shape, so the shape
is a `SceneDef` in `.rodata` and the code exists once.

The limit is honest: a room with genuinely unique behaviour cannot express it
in a table. When the forge minigame lands in Week 2 it goes behind a per-scene
hook, not into `scene.c`.

Textures are unloaded on the way out rather than kept resident for every room.
At 320×240 either choice is cheap now; unloading keeps VRAM flat as the room
count grows, and the reload hides inside the fade that is already playing.

### The fade is not decoration

A hard cut between two full-screen backdrops reads as a dropped frame. 0.28 s
to black and back is enough to say "door", and it is where the texture swap
happens, so the load is never visible. Input is refused while `fade_dir != 0`
— a key press during a transition would otherwise reach a room the player can
no longer see.

## The title screen

Four rows, a placeholder backdrop and no payload. The backdrop is the forge —
`bg_smithy`, already embedded for the room — so the title screen costs a
texture handle and **zero asset bytes**. When real title art arrives it is one
`#include` and one field.

It is deliberately **not** a `Scene`. A room is a backdrop plus actors plus a
script with an entrance state machine behind it; the title has none of those.
Making it a `SceneDef` row would mean teaching `SceneDef` about menus that are
not the command menu, and teaching the scene's ESC ladder about a screen with
nothing to go back to. `main.c` holds two modes instead, and the game's room
is not loaded until START or LOAD is chosen — which is what keeps a save from
being resumed on top of a world that already started.

### LOAD is offered only when it works

`SceneSaveExists()` actually reads the file and checks the magic, the version
and that the room index is in range. A save from before 1.2 has no gold field
and there is no honest default for one — zero robs the player, `GOLD_START`
pays them twice — so it is refused rather than guessed at.

When there is no save, LOAD greys out and the cursor **steps over** it rather
than the row being hidden. A menu that changes length between visits is harder
to learn than one with a row you can see is unavailable.

OPTION is named and says so: *"Options are not built yet."* The row is a
promise about where the setting will live, and an empty submenu behind it
would be the worse lie.

### A load resumes; it does not replay

Entrance and welcome are both skipped, actors start on their marks, and the
phase goes straight to `PHASE_DONE`. Watching two men walk in again on every
load is the kind of thing that makes people stop saving. The script counter in
the blob is not restored either: it records where the player *was*, and a
half-finished conversation is not a place.

## The shop is a conversation now

```
enter the room  ->  JACK's welcome  ->  counter opens
                                          |
                          ESC ------------+
                           |
                    counter closes, JACK: "Anything else?"
                           |
                    YES ---+--- NO
                     |            |
                counter resumes   JACK's parting line -> walk out to the smithy
```

Three separate things fell out of this:

**The welcome opens the counter.** The player travelled here to trade; making
them open a menu to reach the thing the room exists for is a step that only
ever gets in the way.

**ESC replaced 1.3's `END TRADING?` modal.** The shopkeeper asking out loud is
the same confirmation with one fewer piece of furniture, and it is the only
place in the game where a UI decision is voiced by a character. YES is the
default, because a player who reflexively confirms is far likelier to have
more shopping than to have meant to walk out.

**YES resumes rather than reopens.** `ShopResume()` leaves tab, cursor and
sort alone; `ShopOpen()` resets them. From the player's side they never left
the counter — JACK just asked a question over the top of it.

The "and then" is carried by one `AfterScript` byte on the scene, so the
dialogue pump still does not know what a shop is:

| Value | Meaning |
|---|---|
| `AFTER_OPEN_SHOP` | fresh counter |
| `AFTER_RESUME_SHOP` | same counter, state intact |
| `AFTER_ASK_AGAIN` | raise the "anything else?" prompt |
| `AFTER_LEAVE_SHOP` | walk out of the room |

## Prices carry their unit

`POTION 33 G`. An unlabelled `33` beside `POTION` reads as *thirty-three
potions* to somebody at least once, and the shop puts prices and stock counts
in the same column position on different tabs.

The suffix costs two characters of row width, which the longest item names did
not have. Rather than widen the list, `UiRow` now takes a `reserve` and clips
the label to what is left, replacing the last character with a full stop so a
cut name never looks like a real one that ends oddly.

Widening the list was the alternative and it was worse: the detail pane is 23
columns and **every** item description in `game_data.h` is hand-wrapped to
exactly that, so taking pixels from it means rewrapping thirty-odd strings and
living with whatever bad breaks fall out. The full name is on screen anyway —
it is the heading of the detail pane for the selected row.

Of the fourteen stock rows exactly one clips: `Stamina Draught` →
`Stamina Draug.`

## The caret names its key

The blinking chevron says there is more to read. It does not say how to get
it, and a player who has not been told will sit through the reveal waiting for
it to advance by itself. `SPACE` now sits beside it — steady, not blinking,
because text that flashes reads as a warning.

## Sorting

Three keys, each a toggle. There is no sort menu, no submenu and no cursor to
lose:

| Key | First press | Second press |
|---|---|---|
| `R` | rarity, high → low | rarity, low → high |
| `T` | quantity, most → fewest | quantity, fewest → most |
| `A` | name, A → Z | name, Z → A |

The default direction differs per mode because the first press should mean the
useful thing: the best gear, the biggest pile, the top of the alphabet.

**Every mode tie-breaks alphabetically.** Without it a run of six Common items
sits in whatever order `ITEMS` happens to list them, and the player reads that
as a broken sort even though the sort is correct.

### Insertion sort, not qsort

At most a couple of dozen rows. `qsort` costs a function-pointer call per
comparison plus libc's implementation; insertion sort is about forty bytes of
code, is faster at this size, and is stable — which is what lets the
alphabetical tie-break survive.

### One flat row type

`SortRow` is `{name, qty, rarity, ref}`. Flattening the list before sorting is
what lets one sort serve three different screens: the inventory's category
filter, the shop's stock table and the player's pack are three different
shapes, and the sort does not need to know which it is looking at. `ref` is
the caller's own index — an `ItemId` on two of them, a `SHOP_STOCK` row on the
third — and the sort never interprets it.

That indirection caught a bug that would otherwise have shipped: in the BUY
tab **the stock index is not the row index once the list is reordered**.
`s->stock[row]` was correct in 1.2 and silently wrong the moment sorting
existed — you would have bought Coal and watched the Buckler's stock go down.
`RowStock()` now maps back through `ref`, and `test_sort.c` buys a row under
an active sort and checks the right shelf emptied.

The rows are rebuilt on every call rather than cached. At 24 rows the walk is
cheaper than the state needed to know when the cache went stale, and the held
counts change underneath it on every single trade.

### Quantity means different things on different screens

On the BUY tab it is what is on the shelf, not what is in the pack. Sorting a
shop's stock list by the player's holdings would order it by something the
shelf knows nothing about.

### The title screen

Four rows, a placeholder backdrop and no payload. The backdrop is the forge —
`bg_smithy`, already embedded for the room — so the title screen costs a
texture handle and **zero asset bytes**. When real title art arrives it is one
`#include` and one field.

It is deliberately **not** a `Scene`. A room is a backdrop plus actors plus a
script with an entrance state machine behind it; the title has none of those.
Making it a `SceneDef` row would mean teaching `SceneDef` about menus that are
not the command menu, and teaching the scene's ESC ladder about a screen with
nothing to go back to. `main.c` holds two modes instead, and the game's room
is not loaded until START or LOAD is chosen — which is what keeps a save from
being resumed on top of a world that already started.

### LOAD is offered only when it works

`SceneSaveExists()` actually reads the file and checks the magic, the version
and that the room index is in range. A save from before 1.2 has no gold field
and there is no honest default for one — zero robs the player, `GOLD_START`
pays them twice — so it is refused rather than guessed at.

When there is no save, LOAD greys out and the cursor **steps over** it rather
than the row being hidden. A menu that changes length between visits is harder
to learn than one with a row you can see is unavailable.

OPTION is named and says so: *"Options are not built yet."* The row is a
promise about where the setting will live, and an empty submenu behind it
would be the worse lie.

### A load resumes; it does not replay

Entrance and welcome are both skipped, actors start on their marks, and the
phase goes straight to `PHASE_DONE`. Watching two men walk in again on every
load is the kind of thing that makes people stop saving. The script counter in
the blob is not restored either: it records where the player *was*, and a
half-finished conversation is not a place.

## The shop is a conversation now

```
enter the room  ->  JACK's welcome  ->  counter opens
                                          |
                          ESC ------------+
                           |
                    counter closes, JACK: "Anything else?"
                           |
                    YES ---+--- NO
                     |            |
                counter resumes   JACK's parting line -> walk out to the smithy
```

Three separate things fell out of this:

**The welcome opens the counter.** The player travelled here to trade; making
them open a menu to reach the thing the room exists for is a step that only
ever gets in the way.

**ESC replaced 1.3's `END TRADING?` modal.** The shopkeeper asking out loud is
the same confirmation with one fewer piece of furniture, and it is the only
place in the game where a UI decision is voiced by a character. YES is the
default, because a player who reflexively confirms is far likelier to have
more shopping than to have meant to walk out.

**YES resumes rather than reopens.** `ShopResume()` leaves tab, cursor and
sort alone; `ShopOpen()` resets them. From the player's side they never left
the counter — JACK just asked a question over the top of it.

The "and then" is carried by one `AfterScript` byte on the scene, so the
dialogue pump still does not know what a shop is:

| Value | Meaning |
|---|---|
| `AFTER_OPEN_SHOP` | fresh counter |
| `AFTER_RESUME_SHOP` | same counter, state intact |
| `AFTER_ASK_AGAIN` | raise the "anything else?" prompt |
| `AFTER_LEAVE_SHOP` | walk out of the room |

## Prices carry their unit

`POTION 33 G`. An unlabelled `33` beside `POTION` reads as *thirty-three
potions* to somebody at least once, and the shop puts prices and stock counts
in the same column position on different tabs.

The suffix costs two characters of row width, which the longest item names did
not have. Rather than widen the list, `UiRow` now takes a `reserve` and clips
the label to what is left, replacing the last character with a full stop so a
cut name never looks like a real one that ends oddly.

Widening the list was the alternative and it was worse: the detail pane is 23
columns and **every** item description in `game_data.h` is hand-wrapped to
exactly that, so taking pixels from it means rewrapping thirty-odd strings and
living with whatever bad breaks fall out. The full name is on screen anyway —
it is the heading of the detail pane for the selected row.

Of the fourteen stock rows exactly one clips: `Stamina Draught` →
`Stamina Draug.`

## The caret names its key

The blinking chevron says there is more to read. It does not say how to get
it, and a player who has not been told will sit through the reveal waiting for
it to advance by itself. `SPACE` now sits beside it — steady, not blinking,
because text that flashes reads as a warning.

## Sorting resets the cursor

Reordering under a fixed cursor leaves the selection on a different item than
the one the player was looking at. The cursor goes back to the top of the new
order instead.

## Leaving the counter, and talking your way into it

**ESC at the shop now asks.** This is not a contradiction of "no confirmation
per unit" — a mis-bought loaf costs four coin and one key to sell back, while
a counter closed by a reflexive ESC costs the walk back through the menu. The
shop stays open and drawn behind the prompt, so *KEEP TRADING* — the default —
loses neither the tab, the cursor, nor the sort.

It reuses `UiPrompt` unchanged. That widget was written generically in 1.1
rather than as a quit box, and this is the first time that paid.

**`M → TALK` is now how you reach the counter.** In 1.2 it opened the shop
instantly; JACK had a `solo` script that was unreachable in his own room. Now
he speaks first and the counter follows, because a trader who opens his ledger
without a word is a vending machine. The line is deliberately not the welcome:
a shopkeeper who greets you identically every time stops being a person.

The "and then" is carried by one `bool` on the scene, so the dialogue pump
still does not know what a shop is.

### Two men, one clock

The shop needed BEST to walk in and JACK to follow him. 1.2's first cut had
one actor per room and a `PHASE_HOLD` that waited before starting it, which
cannot express "and then somebody else arrives".

`SceneDef` now holds an actor array, and each actor carries a delay measured
from the moment the room opens:

```
  0.45  BEST starts walking in from the right
  1.60  BEST at his mark
  2.95  JACK starts walking in from the left
  4.10  JACK at his mark, both settled, the script starts
```

`PHASE_HOLD` is gone, because the hold *is* BEST's delay. Delays are measured
from the room opening rather than from the previous actor: one clock is what
makes the whole entrance four numbers in a table instead of a chain of
callbacks, and it makes the skip on the accept key a single assignment
(`clock = SettleTime(def)`) instead of a walk over per-actor timers.

The script waits for the *last* actor, so JACK's opening line lands as he
arrives rather than being shouted from off-screen. Actors draw in table order,
which is also the depth order — JACK over the counter he steps out from
behind.

### JACK is shorter, and the numbers are template-matched

`REF-SHOP-COMPOSITE-02.png` shows both men already placed, so the placement
comes from it rather than from taste. It is a **re-render**, though, not the
plate with sprites pasted on — its backdrop differs from `bg_shop` by a mean
absolute error of 10.0 per channel, which is as large as the quantisation
error itself. A pixel diff therefore finds as much changed backdrop as
changed character, and the first attempt at measuring it that way returned a
JACK 113 px wide against a sprite that is 98.

Each sprite is instead template-matched: render it at a candidate height,
slide it over the reference, and keep the position and height with the lowest
mean absolute error **over its own opaque pixels only**, which is what makes
the background drop out of the comparison. Both surfaces have one sharp
minimum:

| | Height | Position | Error | Next best |
|---|---:|---|---:|---:|
| BEST | 184 | (230, 71) | 11.8 | 18.6 at y=72 |
| JACK | 168 | (13, 88) | 9.5 | 11.5 at h=169 |

| | Height | Head top | |
|---|---:|---:|---|
| BEST | 184 | y=71 | clipped at the shin |
| JACK | 168 | y=88 | clipped at the shin |

The height number alone does not read as "shorter", because both are clipped
by the bottom of the screen. The 17 px between their head tops is what does.

The first reference asked for a much shorter JACK — `--h 152` and a 30 px head
gap. The second is the one that ships, so he is now only slightly the smaller
man.

### Two JACK sources, and the pre-quantised one lost

`NPC-JACK-SIZE_614x819-01.png` was mode `P`, already reduced to 228 colours,
with 26.6% of its pixels partially transparent. `-04` is a full RGBA render at
9.5% partial alpha. The pre-reduced source is 388 bytes cheaper after DEFLATE
and looks visibly worse in the room: the wide anti-aliased fringe drags the
backdrop into the silhouette on the way down to 320×240, so his hat reads
grey-green and his glasses close up.

Colour-reducing before the pipeline does not help. `png2c.py` quantises anyway,
and it does a better job from a clean alpha edge than from one already blurred
into a matte. Hand-cleaning at target resolution is worth doing; pre-reducing
at source resolution is not.

### Portraits are two strips, not one

One 40-frame strip would be one texture and one bind. It would also be one
16-colour palette shared between a cream-and-brown smith and a green-hooded
trader, and both faces would pay for it. Two strips of 16 beat one of 16, and
a single strip of 32 costs more payload than the second bind saves.

### Shared page furniture

`UiPageChrome`, `UiRow`, `UiDetail` and `UiNumber` moved from `ui_menu.c` into
`ui.c` so the shop draws the same frame as the inventory instead of a near
copy. The duplicate would have been ~600 bytes; the real cost would have been
the two layouts drifting a pixel apart every time one was touched.

### No confirmation prompt at the counter

One unit per key press, with a result line under the list. A modal per unit
would triple the presses in a shop where the common action is buying eight of
something cheap. The spread is the cost of a mistake and is meant to be felt.

### Save version 2

The blob gains gold, the current room and the held counts — 64 bytes. A v1
save is refused rather than upgraded: the fields it lacks have no safe default
now that coin can be spent.

## Two bugs the merchant art found

Neither was in the new code. Both had been shipping since 1.1 and only became
visible when a second character went through the pipeline.

### png2c.py silently dropped palette alpha

`load_rgba` tested `im.mode == "RGBA"` and otherwise fell through to an opaque
image. `NPC-JACK-SIZE_614x819-01.png` is mode `P` with its transparency in a
tRNS chunk — a normal indexed export — so JACK would have shipped as a sprite
with a solid rectangle of background baked around him. Anything with
transparency is now converted before channels are looked at.

`check_asset.py` had the same test and so rejected the file for the wrong
reason. It now checks the converted alpha.

### The portrait slicer let decorations resize the face

`make_portraits.py` sized each 48×48 frame from that cell's own content
bounding box. A frame with anger marks beside the head measured 244 px wide
against 201 for a plain one, so **the same face was drawn at 1.21× different
scales across the twenty frames** of the shipped 1.1 strip.

Framing is now one decision for the sheet — a single square size, hair line
and offset, taken as medians across all twenty cells — so a sweat drop can no
longer shrink a face. Cell positions come from a grid fitted to the band
starts rather than from the bands themselves, which removes the seam
threshold from the result entirely.

That also made the strip 18% cheaper (9,801 → 8,011 bytes): twenty frames at
one scale share far more runs than twenty frames at twenty scales.

### And the seam threshold is now searched for, not assumed

`MERCHANT-MOOD-01.png` failed the 2% seam threshold that `HERO-BEST-MOOD-01`
was tuned against, and merged four columns into one band.

It is not the canvas size. The deepest interior seam carries 30 lit pixels on
the merchant sheet against 4 on the hero's — and the merchant sheet being the
*taller* of the two dilutes its density rather than raising it. The cause is
the art: JACK's hood tapers down-left past his face and its pom-pom crosses
into the next cell. BEST is bare-headed, so his cells are separated by clean
black.

How empty a seam is is therefore a property of the sheet, not a constant. The
threshold is now searched upward until it resolves exactly the expected cell
count, and both sheets pass:

| Sheet | Row threshold | Col threshold |
|---|---:|---:|
| `HERO-BEST-MOOD-01` | 0.015 | 0.007 |
| `MERCHANT-MOOD-01` | 0.005 | 0.048 |

Framing no longer depends on which threshold wins, so this only has to get the
cell *count* right.

### Framing is still a knob

JACK's hood is a large shape sitting left of his face, so every automatic
centring rule put his face off to one side. `make_portraits.py` takes
`--shift DX,DY` and `--side N`; he ships at `--shift 26,10`. Framing is an art
decision and gets a flag rather than a fifth heuristic.

## Asset validation

`tools/check_asset.py` runs before every conversion (`make check`, or the head
of `build_assets.bat`). It checks backdrops for 4:3 and uniform padding,
characters for a real alpha channel and a matte halo, and mood sheets by
running **`make_portraits.py`'s own band detection** — so a sheet that would
mis-slice fails at the tool with its measured bands printed, rather than as
twenty bled portraits.

It exists because `BG-01-ITEM_SHOP-A01.png` was the second backdrop in a row
where the framing question ("is the shopkeeper baked into the plate?") mattered
more than any pixel-level property, and that is not something the eye reliably
catches at 1448×1086.

## Nothing may overrun its panel

`UiDrawText` walks a fixed 6 px pitch until the string ends. There is no
wrapping and no clipping, so a hint one word too long is simply drawn over the
backdrop outside the frame — which is what the first 1.3 build did:
`ESC LEAVE` came out as `ESC LEA` and the tail sat outside the panel.

Clipping at draw time would hide the bug rather than fix it, and a runtime
length check costs a branch every frame to catch a mistake that is fully known
when the string is written. So the budget is a constant and every hint is
asserted against it where it is defined:

```c
#define HINT_MAX_CHARS (((PAGE_X + PAGE_W) - LIST_X - 4) / FONT_CELL_W)   /* 49 */

#define UI_HINT_FITS(s) \
    _Static_assert(sizeof(s) - 1 <= HINT_MAX_CHARS, \
                   "hint line is wider than the panel: " s)
```

Adding one word too many is now a build error that names the offending string.
Costs nothing at runtime.

| Hint | Chars | Spare |
|---|---:|---:|
| shop | 43 | 6 |
| list | 43 | 6 |
| sort | 34 | 15 |
| travel | 43 | 6 |

The shop hint dropped `U/D PICK`. The list has a highlighted row and a caret,
so the first thing a player tries is the one thing that needs no label; left
and right do need one, because switching tab is not something a list implies.

### And a collision that had not happened yet

Checking the hint line surfaced a worse one on the title line above it. The
sort tag was right-aligned next to the shop's purse — and the purse is
right-aligned too, so the two slide toward each other as the player gets
richer and overlap at **five digits of gold**. Nothing in the demo reaches five
digits, which is precisely why it would have shipped.

The tag moved to a fixed `SORT_TAG_X` on the left, clear of the longest page
title. There is now 120 px of clearance at a six-digit purse.

Widths that are only known at run time — page titles, the tag, the purse,
prompt titles, notes and button rows — are measured in `test_sort.c` rather
than eyeballed.

## Tests

    make test

Three suites, linked against a headless raylib stub and run under
AddressSanitizer and UndefinedBehaviorSanitizer. `DecompressData` returns
NULL, which `GfxLoadTexture` already handles, so no art is decoded and no GPU
is touched. See `tests/README.md`.

The sanitizers are most of the value here: the scene owns four UI layers, a
texture pool and a mutable inventory, and a lifetime bug in that would
otherwise surface as a crash on somebody else's machine.

## A unity-build hazard, and a test harness one

`title.c` defined `TITLE_Y` and `NOTE_Y`. So did `ui_prompt.c`. In a unity
build that is one translation unit, so the second definition silently replaced
the first — the compiler warned only because the *values* differed. Two
screens that happened to agree on a number would have shared it without a
word. Every macro in `title.c` is now prefixed `T_`.

Separately, `make test` crashed rather than reporting. A stale ordering left
the counter open, so `M` was correctly refused, so no dialogue line existed,
so the next line read through a null pointer and took the whole suite down —
including the eleven checks after it that would have passed. `CHECK` records
and continues; the new `REQUIRE` in `tests/check.h` stops the run cleanly when
an assertion is a precondition for the lines below it.

## Manual checklist for 1.4

1. The game opens on the title. LOAD is greyed and **DOWN skips over it**.
2. OPTION says it is not built. EXIT quits.
3. START, buy something, save from the pause box, quit, relaunch — LOAD is now
   live and restores the gold, the pack and the room, with no entrance replay.
4. Travel to the shop: the welcome plays and the counter opens on its own.
5. Switch to SELL, sort by name, move the cursor. ESC. JACK asks. YES puts you
   back on the **same tab, row and sort**.
6. ESC again, NO — JACK says goodbye and you end up in the smithy.
7. Every price reads `<n> G`. `Stamina Draught` shows clipped with a full stop
   and its full name in the detail pane.
8. When a line finishes revealing, the caret blinks next to a steady `SPACE`.

## Manual checklist for 1.3

1. On **M → INVENTORY**, press `R` — best tier first. `R` again — worst first.
   `T` and `A` likewise. The active order is named on the title line.
2. Two items of the same tier are always in alphabetical order.
3. In the shop, sort by name, then buy something in the middle of the list.
   The item you were looking at is the one that arrives, and its stock — not
   some other row's — goes down.
4. `T` in the shop orders by what is on the shelf, not by what you are
   carrying.
5. ESC at the counter asks. *KEEP TRADING* returns you to the same tab, row
   and sort order. ESC again on the prompt also returns you there.
6. *DONE* closes the counter and does not open the pause box.
7. **M → TALK** in the shop plays a JACK line that is not the welcome, and the
   counter opens when he finishes.
8. JACK sets off while BEST is still walking; the dialogue starts at ~2.45 s.
9. `F5` replays the room. `R` no longer does.
10. Every hint line ends inside the panel frame, on all four screens.

## Test checklist for 1.2

1. **M → MAP → Market Row → SPACE** fades to the shop; the fade never shows a
   half-loaded frame.
2. Arrow keys during the fade do nothing.
3. In the shop, **SPACE** on a row buys one; gold falls by exactly the listed
   price and **M → INVENTORY** shows the new count.
4. Buy a row to zero — the price greys out and the result line says sold out.
5. Spend to under the cheapest price — the trade is refused, gold never goes
   negative.
6. On the SELL tab, sell the last unit of an item; the row disappears and the
   cursor does not point past the end of the list.
7. Adventurers Guild, Ore Road and Capital Gate refuse to travel.
8. **ESC** in the shop leaves the counter, does not open the pause prompt.
9. Save, then check `iron.sav` is 64 bytes.
10. Enter the shop and watch: BEST settles at ~1.6 s, JACK walks in from the
    left at ~3.0 s, the script starts at ~4.1 s with JACK's line. JACK's head
    should sit about 17 px below BEST's.
11. Press SPACE during the entrance — both men snap to their marks together,
    not just whoever was moving.
12. JACK's portrait is his own face, not BEST's.

## Next

- Sorting is per screen and resets on quit; it is two bytes, so persisting it
  in the save blob is a version 3 whenever that is worth a bump.
- Shop stock resets on every visit. A day counter in Week 4 makes that a
  restock rather than an exploit.
- `SELL_NUMERATOR` / `SELL_DENOMINATOR` and `RARITY_MUL` are the economy's
  only two knobs.
- `HERO_REST_X` / `HERO_REST_Y` / `ENTER_SECONDS` are the tuning knobs.
- A 1–2 px vertical bob during `SMITHY_ENTER` would sell the walk without a
  real walk cycle. Deliberately left out of 1.0.
- Forge glow and lantern flicker belong in a VFX pass driven off a shared
  scene clock, not baked into `scene_smithy.c`.
