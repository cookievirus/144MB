# IRON & INVESTMENT — Demo 1.9.3

Two rooms, a trader, an anvil, and a day that ends when you say so. One key
opens everything: **M**. Both rooms are now lit.

**1.9.3** made the lamps read as light. The glow was the wrong *shape* — the
falloff only ran vertically, so the sides were cut square — and at a 4 px
radius it was a bright dot on a lamp rather than a lamp throwing light.

**1.9.2** re-measured one lamp and moved it a pixel. It exists as its own
number because the build stamp is on screen now, and two builds that differ
and agree on their stamp make the stamp worse than useless.

**1.9.1** takes the hearth and makes it a room property:

- **Market Row's lamps flicker** — a wall lantern and two hanging lamps,
  steadier and yellower than a forge, each on its own phase.
- **Lights are a table, not a field.** `SceneDef` carries up to four, and a
  lamp is a hearth that throws no sparks.
- **The build stamp sits bottom-right**, from a single `GAME_VERSION`.

**1.9** is two small things, one of which is the first moving thing in the
game that is not a person:

- **The hearth burns.** Embers rise off the coal bed and the light over the
  fire breathes. Zero asset bytes — rectangles, an integer hash and no libm.
- **The date badge got room to breathe**, 64×12 → 82×13.

**1.8** is furniture, and one bug that furniture was hiding:

- **The date badge is a panel**, framed like everything else, instead of a
  bare fill that vanished into the shop's lit shelves.
- **The prompt box measures itself.** It was a fixed 152 px, and 1.7's
  `The forge goes cold until morning.` drew straight out through both walls.
  The box now grows to its widest line and clamps at the screen.
- **The pause box is a column**: SAVE, CANCEL, QUIT, reading in the order the
  actions escalate, with the safe one under the cursor.

**1.7** is about there being one way in:

- **Every feature is a row in the main menu.** The counter no longer opens
  itself when JACK finishes talking, and the anvil no longer answers the
  accept key. **M** gives you TALK, the room's own feature, INVENTORY,
  EQUIPMENT, MAP and END DAY.
- **The feature row follows the room.** FORGE in the smithy, BUY/SELL at
  Market Row, PARTY at the Guild when the Guild exists. One byte on
  `SceneDef` says which.
- **A hint in the corner**, because taking away the two things that opened
  themselves left a room that does nothing when you press anything.
- **Days.** The date sits top right and never leaves. END DAY asks before it
  commits, then fades, turns the world over — restocking the shelves — and
  comes back on the morning after.
- **Numbers carry their unit.** Coin reads `120 G`, counts read `4 EA`.

**1.6** is the forging itself:

- **A sequential QTE.** One key lit at a time — the four arrows and SPACE —
  each with a window that tightens as the sequence goes on. Length comes from
  the tier of the thing being made: five steps for a dagger, seven for a
  staff.
- **The ore is spent when the heat starts, not when it ends.** A ruined heat
  costs exactly what a good one costs. That is the whole stake.
- **Three verdicts.** Flawless promotes the output where the recipe has a
  better version of itself — a clean Iron Shortsword comes off the anvil as a
  Steel Longsword. Within tolerance pays the ordinary item. Past tolerance
  spoils the metal.
- **ESC does nothing while the metal is hot.** The only place in the game
  where the back key is refused outright.
- **No score, no combo, no numbers.** Which key is live, how much sequence is
  left, and how long this step has. Nothing else.

**1.5** gave BEST his own counter:

- **The forge makes things.** Six recipes over two shelves, WEAPONS and
  ARMOR, each one a row that names what it wants and how much of it you have.
- **A recipe you cannot afford stays on the shelf.** It greys, loses its
  quality ball and says which ingredient is short. It does not disappear.
- **BLUEPRINTS is the same list, read-only** — every recipe you know, both
  shelves at once, and an accept key that deliberately does nothing.
- **Three new things to make.** Iron Dagger, Ash Staff and Iron Cuirass are
  seeded at zero: the only way to hold one is to have made it.
- **Save version 3.** The blob gains the known-recipe mask and three held
  counts. A v2 save is refused.

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

### The glow was a lens, not a circle

`VfxBlob` shaded each row by its height and then kept that alpha **flat across
the row's width**. So the top and bottom faded and the left and right ends
were cut off square. It also used a quadratic approximation for the row width
instead of the ellipse, which is narrower at mid-height than a circle.

Both are invisible at a 4 px radius, which is why the hearth got away with it.
At the radius a lamp halo actually wants, it reads as a bar.

**Doing it per pixel** would be a `DrawRectangle` per pixel — about 500 for
one lamp — to compute a falloff the blend can accumulate for free.
**Stacking N flat ellipses** gives a genuinely radial profile in N calls: at
any distance the accumulated alpha is the sum of the layers that still reach
it. Layer alphas are weighted N, N−1, … 1 rather than equal, which makes that
sum quadratic in distance rather than linear — a bright core with a long thin
skirt instead of a flat disc with an edge.

The measured horizontal profile of a lamp, centre outward:

```
37  27  27  19  12  12   7   3   3   1   1   0
```

Row widths now come from the true ellipse, via a nine-iteration binary-search
integer square root. No libm, and obviously correct at a glance, which the
bit-by-bit integer sqrt is not.

This replaced the hearth's two-blob core-plus-halo as well, so its `rx`/`ry`
are the old *outer* pair — the reach, not the core.

### 0.74 was a swing the tuning file could describe and the eye could not see

The lamp flicker floored at 0.74, which sounds gentle and is. But the peak a
lamp adds is around 40 of 255, so floor 0.74 moved the light by **three parts
in a hundred** against the backdrop. Correct in its own units and invisible in
the ones that matter.

Floor 0.62 now, and it visibly breathes onto the panelling around the lantern.
The lesson generalises: an effect's parameters are relative to the effect, and
the thing to check is always the change against what is behind it.

The halo breathes in **size** as well as brightness — ±1.5 px at a lamp's
radius. A light that only changes alpha reads as a lamp on a dimmer; one whose
reach moves with it reads as something burning.

### A lamp is a hearth that throws no sparks

1.9 put one `FireDef` on `SceneDef`, which was right for exactly as long as
one room had one fire. The shop has three lights, so it became a table of four
`LightDef` with a `kind` byte — and the thing worth noting is what *did not*
need a second code path. A lamp is the same bloom in the same blob routine
with the same noise; `embers == 0` already meant "no particles", so the lamp
case fell out of the data rather than being written.

What the `kind` byte buys is tuning, and it earns its place there. A forge is
being fed air: fast, deep, floor 0.58. An oil wick is not: 1.9 Hz and 4.3 Hz,
floor 0.74. One tuning for both makes either the wick roar or the forge sulk,
and `test_vfx.c` asserts the lamp swings less than the hearth rather than
trusting anyone to eyeball it.

**Each light gets its own phase from its index.** Three lamps on one clock
breathe in unison, and that single tell gives the whole effect away as one
timer. The test checks all three differ on essentially every frame.

**No lamp sparks.** Partly because a wick does not, and partly because a lamp
throwing embers in a shop full of potions is a lamp about to end the shop.
There is one ember pool per room, owned by the first light that asks — a
second sparking fire in one room would silently get none, which is a
limitation and not a bug: no room has two forges, and an array of pools sized
for a case that does not exist is the more expensive mistake.

### Finding the lamps was the hard part

The threshold pass that worked on the smithy found the wrong things in the
shop. A doorway full of daylight and a skylight are far brighter than a wick,
so the top blobs were sunlight at (231,129) and (177,203) and the lamps did
not make the list at all.

Ranking by **warmth over luminance** inside the two regions the mock circled
found them. The general lesson, which is now in the GDD: *the brightest thing
in a room is usually a window.*

**And then the method was still too cheap.** 1.9.1 took the single brightest
warm cell in each window — one sample, landing wherever the resample happened
to put a highlight. 1.9.2 takes the warm-weighted centroid of the whole core:

| | 1.9.1 | measured centroid | core |
|---|---|---|---|
| wall lantern | (13, 63) | (12.8, 62.6) | x 4..21, y 53..73 |
| hanging lamp A | (224, 29) | (224.0, 29.0) | x 217..231, y 26..32 |
| hanging lamp B | (253, 28) | (252.2, 29.4) | x 248..255, y 27..33 |

Two of the three were already right, which is the useful part of the result:
the cheap method was not so much wrong as unverifiable, and being right twice
by luck is not a method.

The pin in `test_vfx.c` is now **within a pixel** rather than exact. The
measurement is a centroid over a resampled image and lands on a fraction;
asserting the integer it happens to round to would fail under a different
resampler while telling nobody anything. A pixel is the tolerance that
separates "the glow is on the lamp" from "the glow is on the shelf", which is
the thing worth failing over.

**The radii are still a judgement call.** The lantern's warm core measures 18
px across, but most of that is brass the artist already painted lit, so the
glow is `rx 5` on the flame rather than on the fixture. I cannot check that
from here — if the lantern reads small on screen, `rx 6, ry 7` in `scene.c` is
the one-line change.

### The version number was in four places

Three comments and a README heading, none of which can be wrong in a way that
matters. The moment it went on screen that changed — a stale stamp is a
submission claiming to be a build it is not — so it lives in `src/version.h`
and the banner, the corner and everything else read it from there.

It is never suppressed. A screenshot or a stream of a contest build should say
which build it is without anyone asking, and the moment it *can* be hidden is
the moment the one screenshot that mattered was taken with it hidden. A
`_Static_assert` keeps it clear of the menu hint at the other end of the row.

### Not drawing fire

The backdrop already draws fire, and it draws it better than anything this
module could afford. What it cannot do is move, and a room whose only moving
thing is the player reads as a menu with scenery behind it.

So the effect is deliberately not a fire. It is two cheap moving things laid
over art that is already right: fourteen embers rising off the coal bed, and a
warm bloom over the mouth whose brightness breathes. The alternative — a
six-frame flame loop at the measured 32×22 — is roughly 4 KB of asset for one
room, and it would still visibly loop.

**Additive, not alpha.** Alpha over the smithy's dark beams greys them and
over the shop's lit shelves washes them out. A fire adds light to what is
behind it; nothing else in the game does, and it is the one place
`BeginBlendMode` earns its call.

**The numbers came off the art, not off a screenshot.** A threshold pass over
`resource/BG-01-SMITTY-A03.png` at 320×240, connected-component labelled to
separate the hearth from the lantern and the window, puts the lit mouth at
**x 216..247, y 110..131**. The `FireDef` in `scene.c` centres on (232,121)
because of that measurement, and `test_vfx.c` asserts the glow stays inside
those bounds — so a re-export at a different crop fails a test instead of
silently moving the fire onto the wall.

### Flicker is not a sine

`sinf` would drag libm's trigonometry in for one visual effect, and it is the
wrong shape anyway: a sine flickers like a pulse, and the eye reads a pulse as
a bug. The flicker is smoothstepped value noise over an integer hash, sampled
at 5.3 Hz and 14.7 Hz and summed. The rates are deliberately not multiples of
each other, so the sum does not return to where it started on any interval a
player would sit through.

It floors at 0.58 rather than 0: this is a lit forge, and a fire that can go
fully dark looks like a lamp with a loose connection.

### The generator is private, and that is load-bearing

`vfx.c` has its own hash and does not touch the one in `qte.c`. Sharing would
make a forge sequence depend on how many frames the player spent looking at
the fire, and `test_qte.c` asserts exact sequences under a seed — so the
symptom would have been a test suite that passes alone and fails in the full
run. `test_vfx.c` pins it directly: seed the QTE, run 137 frames of fire, seed
again, and check the sequence is identical.

Embers do not share a generator either. Each derives its spawn from a hash of
its own seed, bumped on respawn, so there is no shared mutable state to
perturb and nothing to reset.

### A ceiling that clips is a particle that blinks out

`rise` started as a hard clip: an ember above the line was skipped. The 30-
second trace in `test_vfx.c` caught it — a slow, long-lived ember climbs 39 px
against a 30 px ceiling, so it was still at a third of its brightness when it
crossed the line and simply stopped existing. A particle that blinks out
mid-air is far more noticeable than one that was never drawn.

It is a fade envelope now. The `continue` only fires where the alpha would
already be zero, and the test asserts the overshoot exists — `lo < top` — so
nobody can quietly turn it back into a clip.

The embers are also scattered up the column at start rather than all born on
the coals, or the first second in a room is one visible pulse.

### Sixty-four pixels was correct and looked mean

1.8 sized the date badge to the tightest box that would hold five digits.
Correct, and it read as a label crammed into its own frame rather than a plate
with something on it. The height is capped by the title-line assertion and
nothing else, so it now takes all of it; the width was free, so the padding
and the gap between `DAY` and the number both roughly doubled. 64×12 → 82×13.

Nothing about the constraint changed — the badge still has to clear
`PAGE_Y + 7`, and the `_Static_assert` still says so.

### A box that trusts its callers is a box that will overflow

`UiPrompt` shipped in 1.1 with `PANEL_W 152` and every caller trusted to keep
its title, its note and its buttons inside that. It held for six versions.
1.7 added a 34-character note — 204 px — and it drew out through both walls.

There *was* a test for it. `test_sort.c` measured prompt titles, notes and
button rows against 152, and it passed, because it measured **a hand-written
list of the strings somebody had remembered to add to it**. The new string was
not on the list. That is the failure mode of every enumerated check: it covers
what you thought of, and the bug is always the thing you did not.

So 1.8 changed the shape rather than adding a sixth string to the list. The
box takes the widest of its title, its note and its button block, adds the
padding, and clamps between a floor — so a two-word question is not a stamp —
and the screen width. The test that replaces the list asserts the *invariant*:
build every prompt the game actually raises, measure the box it produces, and
check the contents are inside it. Then one more with a note longer than
anything shipped, to prove the box follows the string rather than the reverse.

`UI_PROMPT_FITS` catches the remaining case at compile time — a note too wide
even for a full-screen box. Same bargain as `UI_HINT_FITS`: the budget is a
constant, the check is at the definition, and the error names the string.

**What this cost:** `UiPromptWidth` is 323 bytes of measuring that a constant
did for free. That is the price of the class of bug, not of this instance of
it, and it is the right trade at 0.02% of the disk.

### A row is not a list

The pause box was `CANCEL SAVE QUIT` in a row, with CANCEL first because the
safe answer belongs under the cursor. Reading order and cursor position are
different jobs and a row made them fight: the eye reads left to right, so
CANCEL-first says the cancel is the point of the box, when the box exists to
offer a save and a quit.

Stacked, both jobs are satisfied. The list reads in the order the actions
escalate — SAVE, CANCEL, QUIT — and CANCEL is still under the cursor at rest
by being in the middle. Nothing destructive is one keypress from open.

The layout is per-prompt, not global. `NOT YET / END DAY` stays a row, because
a row invites the eye to scan a spectrum and that is exactly right for a
question with two answers and wrong for a menu of unrelated verbs.

`UiPromptMove` now takes both axes and uses the one its layout runs along.
That means a nudge sideways in the pause box cannot silently move the player
off CANCEL onto QUIT, and no caller has to know which layout is on screen to
steer it.

### Twelve pixels

The date badge was a bare `DrawRectangle`, which read fine over the smithy's
dark beams and disappeared into the shop's lit shelves. Every other piece of
standing furniture in the game is a `UiPanel`; this was the one thing floating.

A bordered panel needs eleven rows — border, pad, seven of glyph, pad, border
— and 1.7's argument for the badge never needing to be suppressed was that it
fitted in the 8 px margin above `PAGE_Y`. Those cannot both be true.

What actually has to hold is narrower than "above the frame": the badge must
clear the **title line** at `PAGE_Y + 7`, because that is where the shop draws
its purse and the two would otherwise overlap exactly. Twelve pixels at y=1
ends at 13, and the title line starts at 15. So the badge sits *on* the page
frame's top edge rather than above it, which is what the mock shows anyway,
and a `_Static_assert` holds the two pixels of clearance.

Fixed width rather than measured, so the box does not twitch a glyph wider on
the day the counter reaches ten.

### One entry point, and what it cost

1.4 opened the counter as soon as the welcome finished, on the reasoning that
trading is why the player travelled there. 1.5 gave the anvil the same
courtesy from the accept key. Both were locally right and together they taught
the player that *some* things are in the menu and *some* things happen on
their own — which is the exact confusion a single entry point exists to
remove. A player who has learned that rooms open their own features has not
learned where the menu is, and the Guild will not open its roster by itself.

So both are gone. `AFTER_OPEN_SHOP` is gone with them, TALK is only talk, and
the accept key on an idle room does nothing at all.

That last sentence is a usability hole, and it is why the **M  MENU** hint
exists. It shows only when the room is idle: with a panel up the player has
already found the menu, and with dialogue up the balloon is what the bottom of
the screen is for.

### `has_shop` and `has_forge` became one byte

Two booleans claimed a room could have a counter and an anvil at once. No room
does, and the root grid could not have drawn it if one did — there is a single
slot for what the room is for. `SceneDef.feature` is a `RoomFeature`, the menu
reads its label out of one table, and the scene owns the switch that turns it
into an open counter or an anvil prompt.

`ROOM_FEATURE_PARTY` is in the enum and has a label. It has no room, because
the Guild needs a backdrop and this repo's `.gitignore` is explicit that a
placeholder must never reach a submission — so it waits on art, not on code.
When the art lands it is one byte in a `SceneDef` and one case in the switch.

### The root grid is no longer 2×2

It is five rows or six depending on whether the room has a feature, laid out
two across, and the panel height follows. The feature sits *second* rather
than first: TALK is in the same place in every room, and a menu whose first
entry moves depending on where you are standing is a menu you have to read
before you can use it.

A five-row grid has a hole in the bottom-right. The cursor clamps to the last
real entry rather than parking on nothing — the same rule the inventory list
already followed when a category ran short.

### The day is a fade with a flag

Travel already faded to black and swapped the room. A day boundary fades to
black and keeps it, so `fade_kind` says what the black is hiding and the two
transitions cannot drift apart in length or curve — which is most of why they
read as the same kind of event.

Everything that happens overnight happens at full black, in one function.
Today that is the date and the shop restocking; it is where the drama engine's
overnight events will go, and having a single place for "the world changed
while you were not watching" is the point of having a day boundary at all.

END DAY asks first, with **NOT YET** as the default. A confirmation whose
default is yes is a slower way of not asking. Saying no leaves the menu open
exactly where it was, rather than dumping the player back in the room.

### The date badge never has to be suppressed

It lives in the eight-pixel margin above the page frame — `PAGE_Y` is 8 and
the badge is 8 tall — so it clears every full-screen panel in the game without
a single special case. That is the whole reason it can honestly be described
as always on screen.

### `4` is not `4 EA`

The shop puts a price and a stock count in the same column position on
different tabs. `UiMoney` already labelled the price side in 1.2; 1.7 labels
the other. A bare number there is ambiguous in whichever direction the player
is not currently thinking, and at 320×240 there is no room for a column
heading to disambiguate it.

Counts that are one half of a ratio stay bare. The forge's `34/40` ingredient
rows do not become `34 EA/40 EA`, which is not clearer, only longer.

`ROW_COUNT_RESERVE` is budgeted for `65535 EA` and not for the four digits
that turn up in play, because `held[]` is `unsigned short` and that is what it
can hold. Sizing the column for the observed maximum is the same mistake as
the purse that would have collided with the sort tag at five digits — which
nothing in 1.3 reached either. It costs the row label two characters, paid the
same way as `ROW_MONEY_RESERVE`: the full name is the heading of the detail
pane for whichever row the cursor is on.

### The tests needed a way in too

Four suites opened the counter by pressing the accept key at an idle screen,
because until 1.7 that worked. When the only way in became the menu, four
suites broke in four places for one reason. `tests/drive.h` names the row and
finds it, so the next time the grid gains an entry, nothing at the call sites
moves.

### The minigame never learns what a keyboard is

`QteKey` is an index into a glyph switch, not a raylib key code. The scene
translates its own `dx`/`dy` and accept key into one, which means `main.c`
needed no new bindings at all — the arrows were already `SceneMove` and SPACE
was already `SceneAdvance`, and both now route to the top of the ladder when a
heat is running. It also means the headless tests drive the minigame by
pressing `QK_UP` rather than by faking an input layer.

### The sequence always terminates

A wrong key and an expired window are the same event: a miss that advances the
step. So there is nothing to abandon into, no stall state, and no abandon key
— a player who puts the controller down still reaches a verdict in 3.85
seconds on a five-step heat, and `test_qte.c` asserts exactly that.

This is why `ESC` can be refused rather than confirmed. The README's rule is
that a reflexive back-key must never throw a session away; here the reflexive
back-key would throw away six ore, and the sequence resolves on its own in
under four seconds regardless, so refusing it costs the player nothing they
cannot get by waiting.

### InvForge had to be cut in half

1.5's `InvForge()` took the ingredients and paid out the item in one call.
There is no place in that shape for a minigame: the ore has to leave the pack
before the player knows how it will go, or a bad heat costs nothing and the
QTE is decoration. So it became `InvSpendMaterials()` and `InvGrantItem()`,
and a ruined heat is precisely a spend with no matching grant.

The check stays all-or-nothing. A recipe short on its third ingredient takes
none of the first two.

### Quality is a promoted output, not a tier on the item

The obvious design is "a fine heat produces a Rare version of the same item".
It does not fit the inventory, and the inventory is right: held items are
counts, not instances. The pack knows it holds four of item 12 and has nowhere
to record that one of them came out better.

Per-instance quality would mean an item list rather than a count array, in
`.bss` and in the save blob, plus every screen that shows a row learning about
it. So `RecipeDef` gains one byte — `fine`, an ItemId — and a flawless heat
produces a different item instead of a better one.

**The gap:** only the Shortsword has a `fine` today, because Steel Longsword
was already in `ITEMS` and the other five recipes have no counterpart that
exists. On a dagger, a flawless heat is worth exactly what a scrappy one is.
The mechanism is one byte and one branch; filling the column in is five names,
five descriptions and five tiers, and it is a content task, not a code one.

### The tuning is four constants

| | Value | |
|---|---:|---|
| `Q_BASE_STEPS` | 4 | plus the output's tier |
| `Q_WINDOW` | 0.85 s | the first step |
| `Q_TIGHTEN` | 0.045 s | shaved off each step after it |
| `Q_MIN_WINDOW` | 0.45 s | the floor |
| `Q_TOLERANCE` | `steps / 3` | misses allowed before it is ruined |

Tolerance is a fraction rather than a constant so a longer sequence is not
punished twice for being longer. Integer division floors it, so a five-step
heat allows exactly one miss and a seven-step heat allows two.

Difficulty cannot be authored per recipe. When a boss commission wants a fixed
eleven-step sequence, that is the point at which it earns a byte in
`RecipeDef` — and not before.

### Arrows are rectangles

The font is ASCII and has no arrow glyphs. Adding four would cost a font
rebuild plus four cells to draw shapes that are sixteen `DrawRectangle` calls
here, and every other widget in the game is already built from rectangles.
SPACE is a flat bar rather than the word, because a text label at this size is
four pixels tall and reads as noise.

The live key's window is drawn as a bar under it that eats itself from both
ends. A number would be exact and useless; what the player needs is *now*
getting narrower.

### The anvil is a capability, not a room type

`SceneDef` gains one byte, `has_forge`, beside the `has_shop` it already had.
A room can have a counter, an anvil, both or neither. A `SCENE_TYPE_SMITHY`
enum would have had to be re-taught every time one of those moved, and the
first room that wants both would break it.

The accept key on an idle screen opens whatever the room is for — the shop's
counter, or the smithy's prompt. The intro script does **not** open it. The
shop raises its counter after the welcome because trading is why the player
travelled there; BEST lives in the smithy, and a menu that opens itself every
time he walks through his own door is a menu in the way.

### FORGE / BLUEPRINTS is a UiPrompt

Not a bespoke two-row menu. This is the same shape as PAUSED and ANYTHING
ELSE — a title and a row of labels — and `UiPrompt` was written generically in
1.1 for exactly this. A hand-rolled chooser would have been ~300 bytes to say
what two strings already say. `FORGE` is the default because it is why anyone
walks up to an anvil.

`ESC` on the list pops one level back to that prompt rather than closing the
whole thing, so a player who wanted the other door does not have to walk back
to the anvil to find it.

### One screen, two modes

FORGE and BLUEPRINTS are the same recipe list, the same detail pane and the
same scroll clamp. The differences are that the category tabs are inert in the
book and that the accept key is refused there. Two files would have been two
copies of the row builder and the ingredient readout so that one of them could
ignore a key press.

### Unavailable recipes are shown, not hidden

A menu that changes length depending on what is in the pack cannot be learned.
The player has no way to tell "I cannot afford this" from "this does not
exist", and the row that vanished is the one they were about to plan around.
Disabled rows keep their place, lose their colour, and the detail pane names
the shortfall: `Iron Ore  34/40`.

This is the call the title screen already makes when it greys `LOAD` instead
of removing it. The two screens now agree, which matters more than either
choice on its own.

### An ingredient is two bytes, not one

The obvious saving is a nibble pair — item id high, count low — and it does not
survive contact with the table. There are 27 items and the Cuirass wants forty
ore. Packing would buy 24 bytes across the whole recipe table and cap the
design at fifteen materials in counts of fifteen, which is the wrong trade
this early. Everything else a recipe needs is reached through `out` into
`ITEMS`, which is the same call `StockRow` makes.

### The row index is not the recipe index

The shop shipped this bug in 1.2 and it was only caught when sorting arrived
in 1.3: reading the table by row number acts on whatever landed in that slot
after the reorder. The forge went in with `ref` from the start, and
`test_forge.c` asserts under an active alphabetical sort that row 0 really is
a different recipe from recipe 0 — and then forges it and checks the right
blade appeared.

### Crafting resolves instantly, on purpose

`ForgeCraft()` is the seam the Week 2 sequential QTE goes behind, and nothing
else needs to move when it lands: the row selection, the material check and
the all-or-nothing consume/grant are already on the correct side of it.
`InvForge()` verifies every ingredient before taking any, so a recipe that
comes up short on its third slot cannot leave the player without the ore *or*
the blade.

### The unity build bit again, one level up

1.4's hazard was two files defining `TITLE_Y`. `static` looked like the
defence against that, and it is not one: it scopes a function to its
translation unit, and a unity build has exactly one. `forge.c`'s `BuildRows`,
`ClampCursor` and `Say` collided head-on with `shop.c`'s — the same three
helpers doing the same three jobs for the other list screen.

That collision was at least a hard error, because the signatures differ. Two
screens whose helpers happened to take the same type would have silently
shared one implementation. Every static in `forge.c` is now prefixed `Forge`,
and the rule is the same as for macros: in this build, file scope is not
scope.

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
| **Object total (1.4)** | **89,823** | **6.09% of the 1,474,560 budget** |

### What 1.9.3 added

| Section | Δ bytes |
|---|---:|
| stacked glow, integer sqrt, size breathing | +76 |

Seventy-six bytes, and most of the visible change is in three numbers in the
scene table. The radial falloff cost nothing because the blend was already
doing the work — it just was not being given anything to accumulate.

### What 1.9.2 added

Nothing. One coordinate moved a pixel and the version string changed. It is a
separate number because the stamp is drawn now: a build that differs from
1.9.1 and calls itself 1.9.1 is a screenshot that lies about which build took
it, which is the whole reason the stamp was added.

### What 1.9.1 added

| | Bytes |
|---|---:|
| lamp tuning table, second draw path, per-light phase | ~640 |
| `LightDef` tables for two rooms | ~96 |
| version stamp and `version.h` | ~40 |
| `.eh_frame`, misc | ~224 |
| **Total object delta** | **+1,000** |

Still zero asset bytes. The three shop lamps cost 36 bytes of table and share
every line of code the hearth already had; the +1,000 is almost entirely the
generalisation — a loop where there was a single light, and a tuning table
where the numbers were constants.

### What 1.9 added

| | Bytes |
|---|---:|
| `VfxFireDraw` | 687 |
| `VfxSpawn` | 502 |
| `VfxBlob` | 194 |
| `VfxFireUpdate` | 188 |
| `VfxWave` (hash + smoothstep) | 145 |
| `VfxFireGlow` | 133 |
| `VfxFireStart` / `Stop` / `IsLive` | 154 |
| **vfx code** | **2,003** |
| `FireDef` in the scene table | 20 |
| date badge rework | inlined |
| **Total object delta** | **+2,660** |

Zero asset bytes and zero `.rodata` beyond the twenty in the scene table. The
ember array is 392 bytes of stack, which costs nothing on disk.

`VfxSpawn` at 502 is the surprise — four hash calls and the mid-life scatter,
inlined into both its callers. If the size audit ever needs it back, dropping
the scatter and accepting one dull second on room entry is most of it.

### What 1.8 added

| Section | Δ bytes |
|---|---:|
| `.text` | +1,088 |
| `.eh_frame` | +88 |
| **Total** | **+1,176** |

| Symbol | Bytes |
|---|---:|
| `UiPromptDraw` | 1,345 |
| `UiPromptWidth` | 323 |
| `PromptBtnBlockW` | 134 |
| `UiPromptOpen` / `OpenColumn` | 170 |
| everything else | 152 |
| **prompt widget, whole** | **2,124** |

The widget roughly doubled: one draw path became two, and a constant became a
measurement. Nothing new is in `.rodata` — the pause box reordered its labels
rather than gaining any, and the date badge's strings were already there.

### What 1.7 added

| Section | Δ bytes |
|---|---:|
| `.text` | +1,568 |
| `.rodata` + `.rodata.str*` | +304 |
| `.data.rel.ro` | +64 |
| `.eh_frame` | +104 |
| everything else | +66 |
| **Total** | **+2,106** |

Almost none of it survives as a named symbol — `RootBuild`, `DrawDayBadge`,
`AdvanceDay` and the rest are small enough that `-O2` folded them into their
callers, so `nm` only finds `UiCount` at 75 bytes. The section delta is the
honest number.

Note what came *off*: `SceneDef` lost a byte per room, `AFTER_OPEN_SHOP` and
its switch arm are gone, and two auto-open paths went with them. The +2,106 is
already net of those.

### What 1.6 added

Same method — both objects rebuilt with identical flags and diffed.

| Section | Δ bytes |
|---|---:|
| `.text` | +3,120 |
| `.rodata.str*` | +193 |
| `.data.rel.ro` | +32 |
| `.eh_frame` | +368 |
| everything else | -16 |
| **Total** | **+3,697** |

| Symbol | Bytes |
|---|---:|
| `QteDraw` | 1,460 |
| `QteUpdate` | 240 |
| `QteBegin` | 215 |
| `QteKeyPress` | 203 |
| `InvSpendMaterials` / `InvGrantItem` | 125 |
| everything else | 218 |
| **QTE code** | **2,461** |

`RECIPES` grew by 6 bytes for the `fine` column. The xorshift generator is 21
bytes and replaces both `rand()` and raylib's `GetRandomValue`, neither of
which is now linked; it is also what makes the sequence replayable under a
seed, which is the only reason the grading tests can assert anything exact.

`QteDraw` is again the bulk of it, and again that is where a screen's bytes
go. Unlike `ForgeDraw` and `ShopDraw` it has no twin, so there is nothing to
factor out.

### What 1.5 added

Both versions rebuilt with identical flags on one toolchain and diffed, since
absolute section totals move with the compiler and only the delta is
comparable:

| Section | Δ bytes |
|---|---:|
| `.text` | +3,728 |
| `.rodata` | +64 |
| `.rodata.str*` | +618 |
| `.data.rel.ro` | +112 |
| `.bss` | +48 |
| `.eh_frame` | +488 |
| **Total** | **+5,074** |

Measured per symbol with `nm --size-sort -S`:

| Symbol | Bytes |
|---|---:|
| `ForgeDraw` | 1,975 |
| `ForgeInput` | 367 |
| `ForgeBuildRows` | 190 |
| `InvForge` | 107 |
| `InvCanForge` | 100 |
| everything else | 250 |
| **forge code** | **2,989** |

| Data | Bytes | |
|---|---:|---|
| `RECIPES` | 48 | 6 recipes × 8 |
| three new `ITEMS` rows | 24 | plus their strings |
| `known` mask | 4 | `.bss`, and 4 more in the save blob |
| new `held` slots | 6 | 3 items × 2 bytes |

`ForgeDraw` is two thirds of it, which is where a list screen's bytes always
go — the shop's draw is the same shape and the same order of magnitude. If the
Week 7 audit is tight, the two draws are close enough that a shared
list-page routine is the obvious ~1.5 KB, and the reason it has not been
written yet is that two callers is where that abstraction usually turns out
wrong.

The save blob is 76 bytes.

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
src/forge.{h,c}        FORGE / BLUEPRINTS: recipes, gating, the craft seam
src/qte.{h,c}          the sequential heat: sequence, windows, grading
src/vfx.{h,c}          room lights: hearths that spark, lamps that do not
src/version.h          the one place the build says what it is
tests/drive.h          reach a feature the way a player has to: via the menu
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

Eight suites, linked against a headless raylib stub and run under
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

## Manual checklist for 1.9.3

1. Each shop lamp has a soft **round** halo — no square left and right edges,
   no bar shape.
2. Watch one for ten seconds: it breathes, in reach as well as brightness,
   and the glow spreads visibly onto the wood at its brightest.
3. It is still light enough to read as a lamp, not a spotlight — the backdrop
   under it is warmed, not washed out.
4. The hearth is unchanged in feel but smoother at its edge.

## Manual checklist for 1.9.2

1. The corner reads **1.9.2**. If it reads 1.9.1 or nothing at all, this is
   not the build you think it is — which is the check the stamp exists to
   provide.
2. The right-hand hanging lamp's glow is centred on its glass, not offset
   toward the shelf beside it.

## Manual checklist for 1.9.1

1. Travel to Market Row. The wall lantern on the left and both hanging lamps
   above the door glow and flicker.
2. Watch them together for ten seconds — they must **not** pulse in time with
   each other.
3. They are visibly steadier than the smithy's hearth. Compare directly.
4. No embers anywhere in the shop.
5. Both figures stand in front of the lamps, not behind them.
6. `1.9.1` sits bottom-right in every room, over every panel, and does not
   collide with `M  MENU` at the other end.
7. The console banner prints the same number as the corner does.

## Manual checklist for 1.9

1. Stand in the smithy and watch the hearth for ten seconds. Embers rise off
   the coals, drift, and **fade out** — none of them blinks off mid-air.
2. The light over the mouth breathes and never fully darkens.
3. It does not repeat on any obvious cycle.
4. Open **M**, the forge list, the QTE. The fire keeps burning behind all of
   them.
5. BEST occludes the near edge of the glow — the effect is behind him, not
   painted over his apron.
6. Travel to Market Row. **No fire there**, and no stray embers.
7. Come back. It is lit again, already running, not starting from cold.
8. The date badge is visibly wider with air around both `DAY` and the number,
   and still clears the shop's purse on the title line.

## Manual checklist for 1.8

1. In the smithy, the date sits top right in a bordered box, over the frame's
   top edge, not floating on the artwork.
2. Open **M → BUY/SELL**. The badge is still legible and does **not** touch
   the purse on the title line.
3. Open the forge list and the QTE. The badge clears both.
4. **M → END DAY.** Every character of `The forge goes cold until morning.`
   is inside the box, which is visibly wider than the other prompts.
5. **ESC** in an idle room. The pause box is a stack: SAVE, CANCEL, QUIT,
   with CANCEL highlighted, all three buttons the same width.
6. Left and right do nothing to it. Up and down move, and wrap.
7. Choose SAVE — `Progress saved.` appears under the buttons, inside the box,
   and the box grows a line taller to hold it.
8. Up and down do nothing to `NOT YET / END DAY`; left and right move it.

## Manual checklist for 1.7

1. Load into the smithy. `DAY 1` sits top right; `M  MENU` sits bottom left.
2. Let BEST finish talking, then press **SPACE**. Nothing happens — correct.
3. **M**. Six rows: TALK, FORGE, INVENTORY, EQUIPMENT, MAP, END DAY.
4. FORGE opens THE ANVIL. **ESC**, **ESC** back to the room.
5. Travel to Market Row. Let JACK finish — the counter does **not** open.
   **M** now shows **BUY/SELL** where FORGE was.
6. In BUY/SELL: prices read `18 G`, the purse reads `… G`, and HELD and STOCK
   in the detail pane read `… EA`.
7. **M → INVENTORY**: every row's count reads `… EA`.
8. **M → END DAY**. It asks, with NOT YET selected. Press **SPACE** —
   nothing changes and the menu is still there.
9. **END DAY** again, move right, **SPACE**. Fade out, fade in, `DAY 2` top
   right, a line about the coals being banked, and the shop's shelves are
   full again.
10. The badge stays readable over the shop page, the forge list and the QTE.
11. Save on day 2, quit, relaunch, LOAD — it is still day 2.

## Manual checklist for 1.6

1. FORGE → WEAPONS → Iron Shortsword → **SPACE**. The room dims, five keys
   appear, the first one glows and a bar under it starts shrinking.
2. Ore and coal are already gone: back out afterwards and check
   **M → INVENTORY**.
3. Hit all five in order — the verdict reads `FINE WORK` and the pack gains a
   **Steel Longsword**, not a Shortsword.
4. Do it again and miss one — `IT WILL DO`, and it is a Shortsword.
5. Miss two — `RUINED`, and nothing is in the pack. The ore is still gone.
6. Start a heat and press nothing at all. It resolves itself in under four
   seconds and ruins.
7. **ESC** and **M** during a heat do nothing at all.
8. No two adjacent keys in a sequence are ever the same.
9. Forge a dagger flawlessly — you get a dagger. This is the known content gap
   above, not a bug.
10. The verdict clears itself and the list comes back on the same shelf, same
    cursor and same sort, with the verdict as its result line.

## Manual checklist for 1.5

1. In the smithy, once BEST stops talking, **SPACE** raises **THE ANVIL** with
   FORGE selected.
2. FORGE opens on WEAPONS. **left/right** switches to ARMOR.
3. On ARMOR, **Iron Cuirass is visible and grey** with no quality ball, and
   its detail pane reads `Iron Ore 34/40` in red plus `SHORT ON MATERIALS`.
   **SPACE** on it says so and spends nothing.
4. Forge an Iron Dagger: ore drops by 2, leather by 1, and **M → INVENTORY →
   GEAR** shows one Iron Dagger that was not there before.
5. Sort the shelf with **A**, then forge the top row — the blade you were
   looking at is the one that appears.
6. **ESC** returns to THE ANVIL, not to the room. **ESC** again leaves.
7. BLUEPRINTS lists all six recipes across both shelves, left/right does
   nothing, and **SPACE** never makes anything.
8. **M** is refused while either list is open.
9. Save, quit, relaunch, LOAD — the forged dagger is still in the pack.
10. Travel to Market Row: **SPACE** there opens JACK's counter, not an anvil.

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

## A fresh clone builds again

`assets/merchant_idle.h` and `assets/portraits_merchant.h` were in
`.gitignore`, dating from when they were magenta placeholders that must never
reach a submission. They stopped being placeholders in 1.2 and the ignore
stayed, so every clone failed on a missing include before it could run a
single test — while the other five generated headers in `assets/` had been
committed all along.

All seven are committed as of 1.6. `make assets` still reproduces them byte
for byte from `resource/`, so nothing is lost by carrying them, and a repo
that cannot be built by the command its own README names is worse than a repo
carrying 13 KB of derived data.

The design intent lives in `docs/GDD.md`. Where the two disagree, the GDD is
the intent and this file is the record of what was actually done.

## Next

- Nothing happens overnight except a restock. `AdvanceDay` is one function and
  it is where the drama engine goes.
- The day has no end. A contest submission needs a last day and a reason to
  have reached it.
- Five of six recipes have no `fine` output, so a flawless heat on them is
  worth nothing extra. Five items is the fix.
- The QTE has no sound. A hit and a miss are the two events in the game that
  most want one, and Week 5's audio pass should start there rather than with
  ambience.
- Recipes are all known from the start. The mask and the save field are
  already there, so teaching one in Week 3 costs a bit, not a save version.
- `ForgeDraw` and `ShopDraw` are the same screen twice. A third list screen is
  the point at which that stops being a coincidence.
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
