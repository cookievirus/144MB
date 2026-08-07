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

The overflow checks measure every string the UI draws against the frame it is
drawn into. Hint macros are already guarded by `_Static_assert` at their
definition; these cover the widths only known at run time — page titles, the
sort tag, the purse at six digits, and the prompt box's title, note and button
row.

These link the `src/*.c` files directly rather than going through `main.c`, so
a test can reach a static helper like `BuildRows` or `SettleTime`. That is
deliberate: the alternative is exporting internals purely to test them.
