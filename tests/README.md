# Headless tests

The game logic is exercised without a window. `raylib_stub.c` supplies the
three dozen raylib symbols the build references; `DecompressData` returns
NULL, which `GfxLoadTexture` already handles by returning an id-0 texture, so
no art is decoded and no GPU is touched.

Everything below runs under AddressSanitizer and UndefinedBehaviorSanitizer,
which is most of the value: the scene owns four UI layers, a texture pool and
a mutable inventory, and a lifetime bug there would otherwise only show up as
a crash on someone else's machine.

    make test

| File | Covers |
|---|---|
| `test_shop.c` | prices, the buy/sell spread, stock exhaustion, the coin floor, the shrinking sell list, travel, the ESC ladder |
| `test_entrance.c` | two-actor entrance timing, overlap, skip-on-accept, portrait routing |
| `test_sort.c` | all six sort orders, tie-breaks, buying under a sort, the trade prompt, TALK into the counter, panel overflow |
| `test_title.c` | START / LOAD / OPTION / EXIT, the greyed row the cursor steps over, save round-trip, resume-not-replay |
| `test_forge.c` | the recipe table, material gating, disabled rows staying visible, the row-to-recipe mapping under a sort, the read-only guarantee on BLUEPRINTS, the unlearned-recipe path, the ESC ladder, save v3 |
| `test_day.c` | the root grid per room, the feature row and its label, the ragged-row cursor clamp, nothing opening itself, the END DAY confirmation both ways, the overnight restock, the date surviving a save, and the unit column widths |
| `test_vfx.c` | hearth and lamp positions matching the measured art, embers staying in their room over 30 s, the flicker moving and staying in range, lamps swinging less than the forge and each holding its own phase, the fade envelope's overshoot, the effect never touching the QTE's generator, and the version stamp clearing the menu hint |
| `test_qte.c` | sequence generation and its no-repeat rule, determinism under a seed, all three verdicts at the tolerance boundary, an unplayed heat resolving itself, the `fine` output ladder and its fallback, the full spend-play-grant cycle driven through `SceneMove` / `SceneAdvance`, and ESC being refused mid-heat |

The overflow checks measure every string the UI draws against the frame it is
drawn into. As of 1.8 the prompt half of that is an *invariant* check rather
than a list: it builds every prompt the game raises, measures the box each one
produces, and asserts the contents are inside it. The list it replaced passed
while 1.7's END DAY note drew out through both walls, because the note was not
on the list - which is the failure mode of every enumerated check. Hint macros are already guarded by `_Static_assert` at their
definition; these cover the widths only known at run time — page titles, the
sort tag, the purse at six digits, and the prompt box's title, note and button
row.

These link the `src/*.c` files directly rather than going through `main.c`, so
a test can reach a static helper like `ShopBuildRows`, `ForgeBuildRows` or
`SettleTime`. That is deliberate: the alternative is exporting internals purely
to test them.

`drive.h` is included after `scene.c` by every suite. It reaches a feature the
way a player now has to - by naming a root row and letting the helper find it
- so a grid that gains an entry does not break four suites in four places, as
1.7 did before it existed.

Two suites need the sequence to be reproducible. `QteSeed()` exists for that
and for nothing else: without it the grading tests could only assert
statistical properties, and "usually ruins" is not an assertion.
