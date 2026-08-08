# IRON & INVESTMENT — Game Design Document

**Status:** working document · **Version:** for Demo 1.9.3 → submission
**Deadline:** 2026-09-04, 23:39 · **27 days from 2026-08-08**

This is a control document, not a pitch. Its job is to make design decisions
**findable and changeable in one place**, so a number is never edited in the
code and forgotten here, or argued about twice.

Three markers are used throughout and they are load-bearing:

| | Meaning |
|---|---|
| **`[BUILT]`** | in the code today, numbers below are read from it |
| **`[LOCKED]`** | decided, do not reopen without editing this file first |
| **`[OPEN]`** | needs a call — listed again in §9 |

> **1.9.3** fixed the shape of the glow — the falloff ran only vertically, so
> the sides were square — and sized the lamp haloes to what a lamp throws
> rather than to its lit glass. +76 bytes. Size 7.65% → **7.66%**.
>
> **1.9.2** moved one lamp a pixel after re-measuring it properly, and took a
> version number of its own to do it — the stamp is on screen, so two builds
> that differ cannot share one. No design change, no size change.
>
> **1.9.1** lit Market Row too — a wall lantern and two hanging lamps — and
> put the build number in the corner. The design content is one line: room
> lights are a **table on `SceneDef`**, not a field, so the Guild's brazier is
> a table row. Size 7.58% → **7.65%**.
>
> **1.9** lit the hearth — rising embers and a breathing bloom over the
> smithy's fire, at zero asset bytes — and widened the date badge. It is the
> first moving thing in the game that is not a person, and it is worth one
> line in §7: it came from Tier 3, item 15's neighbourhood, and cost about two
> hours. The ladder note below still stands; two hours is not three days, and
> the difference is the whole argument. Size 7.40% → **7.58%**.
>
> **1.8** is a furniture pass and changes no design: the date badge got a
> frame, the prompt box learned to size itself after 1.7's END DAY note drew
> outside it, and the pause box became a stacked SAVE / CANCEL / QUIT. Size
> 7.32% → **7.40%**. Nothing below moves as a result; the notes are in
> `README.md`. The one thing worth carrying into §10 is *why* the overflow
> shipped, because it will recur: the test that should have caught it
> enumerated the strings it knew about.
>
> **Changed in the revision before this one (1.4 → 1.7).** Four things moved and three of them
> contradict what this file said. They are called out where they sit, and
> collected in §11 so nothing is quietly overwritten:
> the forge minigame was **built** after §9.1 recommended against it;
> **EQUIPMENT lost its mechanic** as a direct consequence;
> the shop **now restocks twice**, so §9.6 got worse rather than better;
> and every feature moved into the main menu, which is now a `[LOCKED]` rule
> the unbuilt Guild has to follow.

---

## 1. The game in one paragraph

You are BEST, a blacksmith. You cannot fight the Demon King and you are never
going to. What you can do is **forge the weapon that kills him and own a share
of the party that carries it.** Every day you buy materials, work the forge,
and stake gear and supplies on adventuring parties in exchange for a cut of
what they bring back. You never see the dungeon. You read the reports.

**Win** — a party you sponsored kills the Demon King. **Lose** — you cannot
pay your upkeep.

The fantasy is not heroism. It is **venture capital with an anvil**.

### Why this premise is worth defending

Two things fall out of it that a normal RPG cannot do, and both are cheap to
build:

- **The player's agency is indirect.** You choose *what to make* and *who to
  back*, then you watch. That makes a text-driven drama engine the main event
  rather than a consolation prize for having no combat system — which matters
  a great deal when the budget is one floppy disk.
- **Failure is legible.** A party dies and you can point at the corner you cut
  on their gear. That is a better story beat than a game-over screen.

---

## 2. Status board — read this first

**Honest assessment: the player can now make things, and still cannot invest
in anything.** Contest judging is *(1) did you finish it, (2) is it under
size, (3) is it fun.* Size is at **7.66% and is not a risk.** Everything below
the line is criteria 1 and 3.

The half of the loop that faces the anvil is done. The half that faces the
Guild does not exist, and that half is the premise.

### `[BUILT]` — Demo 1.9.1, 5,270 lines of C in `src`, 7,106 with tests

| System | Notes |
|---|---|
| Title screen | START / LOAD / OPTION / EXIT, save-aware |
| Scene table | rooms as `SceneDef` data, two-actor entrances, fades, travel |
| Dialogue | balloon, portrait sets, typewriter reveal, 20 moods per speaker |
| Command menu | screen stack, inventory, equipment, map |
| Quality tiers | 8 tiers, ball + tint, and they drive every price |
| Shop | buy/sell, per-visit stock, conversation loop |
| Inventory | live held counts, gold, learned-recipe mask |
| Sorting | rarity / quantity / name, both directions |
| Save/load | versioned blob (**v4**), gold + pack + recipes + room + day |
| **Forge** | **1.5** — 6 recipes, 2 shelves, material gating, blueprints |
| **Forge minigame** | **1.6** — sequential QTE, 3 verdicts, promoted output |
| **Day cycle** | **1.7** — End Day, confirmation, overnight hook, date badge |
| **One entry point** | **1.7** — every feature is a main-menu row |
| Modal prompts | **1.8** — self-sizing box, row or column layout |
| Ambient VFX | **1.9.1** — up to four lights per room, hearths and lamps |
| Tests | **7 suites** under ASan + UBSan |

### NOT BUILT — this is still the game

| System | Est. | Without it |
|---|---|---|
| **Upkeep** | 0.5 day | days pass but idling costs nothing, so nothing is at stake |
| **Guild scene** | 2 days | parties have nowhere to be hired |
| **Party sponsorship** | 3 days | the player cannot invest in anything |
| **Drama tick + report** | 3 days | nothing happens between days |
| **Event content** | 4 days | the drama engine has nothing to say |
| **Win / lose / ending** | 1 day | it is a toy, not a game |

**13.5 developer-days of unbuilt game in 27 calendar days.** Down from 15 in
28 — three weeks of calendar bought one and a half days of ladder, because
the days went to the two systems that were already half-designed rather than
the four that were not. §7 and §8 exist because of that ratio, and §8 has been
rewritten around it.

---

## 3. The core loop `[LOCKED]`

One day is one turn. The **End Day** button is the only thing that advances
time — no real-time clock anywhere. `[BUILT]` as of 1.7, less the upkeep.

```
   ┌─ MORNING ──────────────────────────────────────────┐
   │  Forge.  Spend materials -> a QTE -> gear.   [BUILT]│
   ├─ DAY ──────────────────────────────────────────────┤
   │  Market Row (JACK): buy materials, sell     [BUILT] │
   │  surplus.                                           │
   │  Guild: review parties, stake gear + supplies for   │
   │  a share of their return.                    NOT YET│
   ├─ END DAY  [button] ────────────────────────── [BUILT]
   │  Drama tick. Sponsored parties act off-screen.NOT YET│
   │  Events fire. Gold, morale and casualties resolve.   │
   ├─ EVENING ──────────────────────────────────────────┤
   │  Read the reports. Pay upkeep. Someone did not NOT YET
   │  come back, or someone came back rich.              │
   └────────────────────────────────────────────────────┘
                          ↓ repeat
```

**Why async ticks and not real time:** the player's job is judgement, not
reflexes. A clock that runs while they are reading an item description is
punishing them for the thing the game is asking them to do.

**Upkeep exists so that idling loses.** Without a daily cost, the optimal play
is to hoard and never risk anything, which is the exact opposite of the game.
**The day cycle shipped without it**, so today ending the day is free and the
optimal play really is to hoard. It is half a day's work and it is the
cheapest thing on the board that turns the clock into a threat. See §9.5.

### One entry point `[LOCKED]` — new in 1.7

**Every feature a room offers is a row in the main menu, and nothing opens
itself.** `SceneDef.feature` is one byte naming what the room is for; the
menu draws its label; the scene owns what it opens.

Rooms as they stand: Smithy → `FORGE`, Market Row → `BUY/SELL`,
Guild → `PARTY` *(label and enum exist; the room does not)*.

This is `[LOCKED]` and it constrains unbuilt work: **the Guild's sponsor flow
is reached from the menu like everything else.** The rule exists because 1.4
opened the counter on its own and 1.5 answered the accept key at the anvil,
and between them they taught the player that some things live in the menu and
some things happen by themselves — so they never learned where the menu was.
A `M  MENU` hint in the corner replaces both.

---

## 4. Systems

### 4.1 Economy `[BUILT]`

Read from `src/game_data.h` and `src/rarity.c`. These are live numbers.

Every price in the game is `base × RARITY_MUL[tier] ÷ 8`:

| Tier | ×    | base 40 → |
|---|---:|---:|
| Junk | 0.25 | 10 G |
| Common | 1.00 | 40 G |
| Uncommon | 1.50 | 60 G |
| Rare | 2.50 | 100 G |
| Epic | 4.00 | 160 G |
| Legendary | 7.00 | 280 G |
| Myth | 12.00 | 480 G |
| Cursed | 3.00 | 120 G |

- Starting purse **250 G**
- Trader buys back at **half** — one flat rate, because the interesting
  decision is what to forge, not where to dump surplus charcoal
- **27 items**: 12 materials, **10 gear**, 5 consumables — up from 24 in 1.4.
  The three new ones (Iron Dagger, Ash Staff, Iron Cuirass) are seeded at zero
  and are not for sale, so **the only way to hold one is to have forged it.**
  That is the first thing in the game that is yours because you made it.
- 14 shop stock rows, restocked **per visit and again at dawn** `[OPEN]` — see
  §9.6, which 1.7 made worse rather than better
- Coin renders `120 G`, counts render `4 EA`, everywhere `[BUILT]` 1.7

**Retuning a whole tier is one byte in `RARITY_MUL`.** That is the single most
valuable property of the economy and nothing should be added that breaks it.

`[OPEN]` **Daily upkeep.** Proposal unchanged: `12 G + 2 G per sponsored
party`. At 250 G start that is ~20 idle days before bankruptcy — enough rope
to learn, short enough to bite. Now blocking, because the day cycle it hangs
off is built.

### 4.2 The forge `[BUILT]` — and the recommendation in this file was overruled

**§9.1 asked table roll or minigame, recommended the table roll, and the
minigame was built.** That is recorded here rather than edited away, because
the reasoning that produced the recommendation was wrong in an instructive
way: the estimate was 3 days for a new input mode, a new screen, a difficulty
curve and its own art. It cost roughly one, because the UI layer already
existed, the sequence draws from rectangles rather than art, and the
difficulty curve turned out to be four constants.

**What shipped (1.5 + 1.6):**

- Six recipes over two shelves, WEAPONS and ARMOR. A recipe is 9 bytes:
  output, shelf, promoted output, and three `{item, count}` ingredients.
- **Unaffordable recipes are shown disabled, never hidden**, with the
  shortfall named: `Iron Ore 34/40`.
- BLUEPRINTS is the same list read-only, gated on a 32-bit learned mask that
  is already in the save — so teaching a recipe later costs a bit, not a save
  version.
- **The ore is spent when the heat starts, not when it ends.** A ruined heat
  costs exactly what a good one costs. That is the entire stake.
- Sequence length is `4 + output tier`, so a Rare staff is seven steps and a
  Common dagger is five. Windows start at 0.85 s and tighten 0.045 s per step
  to a 0.45 s floor. Tolerance is `steps / 3` misses.
- Three verdicts: **flawless** promotes the output where the recipe has a
  better version of itself, **within tolerance** pays the ordinary item,
  **past tolerance** spoils the metal and pays nothing.

**Quality is a promoted output, not a tier on the item.** Held items are
counts, not instances — the pack knows it holds four of item 12 and has
nowhere to record that one came out better. Per-instance quality would mean an
item list in `.bss` *and* in the save blob, plus every row-drawing screen
learning about it. One byte per recipe buys the same drama.

`[OPEN]` **Five of six recipes have no promoted output**, because only Steel
Longsword already existed as a better version of something. A flawless heat on
a dagger is worth exactly what a scrappy one is. Five items — names,
descriptions, tiers — is the fix, and it is content, not code. See §6.

The live table makes it worse than that summary suggests:

| Recipe | Output | Flawless gives |
|---|---|---|
| Iron Shortsword | Common | Steel Longsword — **Uncommon** |
| Ash Staff | **Rare** | *nothing* |
| Iron Cuirass | Uncommon | *nothing* |
| Chain Coif | Uncommon | *nothing* |
| Iron Dagger | Common | *nothing* |
| Buckler | Common | *nothing* |

**The longest, hardest sequence in the game pays no bonus for perfection.**
Sequence length is `4 + tier`, so the Rare staff is the seven-step heat — the
one most likely to be missed and the only one where being flawless is worth
nothing. The cheapest recipe is the only one that rewards skill. That is
backwards, and it is one byte and one item to fix.

`[OPEN]` **EQUIPMENT now has no mechanic at all.** §4.2 as written had the six
tool slots feeding `tool_bonus` into the roll. The roll is gone, the QTE reads
nothing but the player's timing, and `EQUIPMENT` is referenced by exactly one
file — the screen that displays it. Six slots that display "EMPTY" forever are
worse than no slots. Three ways out, in ascending cost:

1. **Tools widen the window.** A fitted hammer adds ~0.1 s per step; a fitted
   anvil raises tolerance by one miss. Two lines in `qte.c`, reads the tier
   that is already in the table. **Recommended.**
2. Tools unlock shelves or recipes. More design, more content.
3. Cut the EQUIPMENT screen. Cheapest in bytes, and admits the six slots were
   speculative.

### 4.3 Party sponsorship `[OPEN]` — unchanged, and now the critical path

Nothing here has moved since 1.4 and everything else has, so this is now the
largest single risk in the project.

A party is a name, 3–5 members, a strength rating, and a personality that
biases which drama events they draw.

```c
typedef struct Party {
    const char *name;
    unsigned char strength;    /* 1..10, drives the event tier band */
    unsigned char morale;      /* 0..100, moves on events            */
    unsigned char greed;       /* how big a cut they demand          */
    unsigned char alive;       /* members still standing             */
    unsigned char gear[3];     /* ItemId of what you staked          */
    unsigned char days_out;
} Party;
```

The player stakes gear and supplies; the party returns a share. Better gear
raises effective strength, which moves them into higher event bands, which pay
more and kill more.

**The central tension:** your best blade in the hands of a greedy party earns
less than a mediocre blade with a loyal one. `[OPEN]` — whether greed is
visible before you commit. Hidden is more interesting and more frustrating.
Proposal: **visible, because a 30-day game does not give the player enough
repetitions to learn a hidden stat.**

**Two things the 1.5–1.7 work already decided for this system**, and they
should be reused rather than re-argued:

- Staking gear is `InvSpendMaterials`-shaped: check everything, then take
  everything, or take nothing. A half-staked party is the same bug class as a
  half-consumed recipe.
- The sponsor flow is a **menu row in the Guild**, per §3. It is not a thing
  that opens when the guild-master finishes talking.

### 4.4 Drama engine `[OPEN]` — schema must be locked before content

This is the game's voice. It is also the one place where **getting the shape
wrong is unrecoverable**: code refactors, sixty hand-written events do not.

```c
typedef struct DramaEvent {
    const char *text;          /* hand-wrapped to the balloon, 39 cols x 3 */
    unsigned char band_min;    /* party strength range that can draw this  */
    unsigned char band_max;
    signed char   gold_pct;    /* % of the party's haul, +/-               */
    signed char   morale;      /* delta                                    */
    unsigned char casualty;    /* 0..255 chance one member does not return */
    unsigned char tags;        /* bitfield, see below                      */
} DramaEvent;
```

`tags`: `COMBAT | TRAVEL | SOCIAL | DUNGEON | GEAR | OMEN` — one bit each,
used to bias draws by party personality and to let the finale require an
`OMEN` chain.

**Size is not the constraint here, and the measurement says so loudly.**
Using the shipped item descriptions as the text-length proxy (71 chars
average, hand-wrapped to the same balloon):

| Events | Struct | Text | Total | % of budget |
|---:|---:|---:|---:|---:|
| 60 | 960 B | 4,250 B | 5,210 B | **0.35%** |
| 80 | 1,280 B | 5,666 B | 6,946 B | **0.47%** |
| 500 | 8,000 B | 35,416 B | 43,416 B | **2.94%** |

Even 500 events costs under 3% of the disk. The constraint is **writing
time**: 500 events at three minutes each is 25 hours, alongside four unbuilt
systems, in under four weeks. See §6.

**Where the tick goes is already built.** `AdvanceDay()` in `scene.c` runs at
full black during the End Day fade, and is the one moment the world is allowed
to change without the player watching. It currently advances the date and
restocks the shop. The drama tick belongs there and nowhere else.

`[OPEN]` **The evening report needs a screen.** The cheapest honest option is
the existing dialogue balloon: the reports are text, the balloon already
word-wraps and reveals, and a party's name plate is a speaker plate. That is
zero new UI. A dedicated report page is nicer and is a day. **Recommendation:
balloon**, and spend the day on events.

### 4.5 Win and lose `[LOCKED]`

- **Win** — a sponsored party kills the Demon King. Gated behind: a weapon of
  Legendary or better, a party at full strength, and an `OMEN` event chain.
- **Lose** — gold below zero at evening upkeep with nothing left to sell.
- **Soft fail** — running out of days. `[OPEN]`: 30 days proposed.

A win must be **reachable in one sitting**, because a contest judge plays once.
Target full run: **25–40 minutes.**

**Note against the win condition:** no recipe in the game produces a Legendary
weapon, or anything above Rare. Either a late recipe exists that does, or the
gate moves. This is not an oversight to fix silently — it is §9.8.

---

## 5. Screens

| Screen | Status |
|---|---|
| Title | `[BUILT]` |
| Smithy | `[BUILT]` |
| Market Row | `[BUILT]` |
| Inventory / Equipment / Map | `[BUILT]` |
| **Forge panel** | **`[BUILT]`** — recipe list, gating, blueprints |
| **Forge minigame** | **`[BUILT]`** — sequential QTE overlay |
| Guild | not built — party list, sponsor flow |
| Evening report | not built — the drama tick's output, see §4.4 |
| Ending | not built |

`DESTINATIONS` already carries Adventurers Guild, Ore Road and Capital Gate as
`SCENE_NONE`. The Guild is the only one that has to exist.

**The Guild needs a backdrop and nothing else is blocking it.** The
`.gitignore` in this repo is explicit that a placeholder must never reach a
submission, so the room waits on art, not on code — `ROOM_FEATURE_PARTY` and
its menu label already work. Art for one room is the smallest unblocking
action available and it is on the critical path.

---

## 6. Content budget

| | Ships | Bytes | Writing |
|---|---:|---:|---:|
| Drama events | **60–80** | ~7 KB | ~4 h |
| Parties | 6–8 | <1 KB | ~1 h |
| Forge recipes | **6 `[BUILT]`**, target 10–12 | <1 KB | ~1 h |
| **Promoted outputs** | **5 missing** | ~0.6 KB | ~1 h |
| Items | 27 `[BUILT]` | — | — |
| Guild backdrop | 1 | ~18 KB | — |

**The "500+ events" target from the original plan is cut.** It was never a
size problem; it is a time problem, and 60 well-written events that fire in
the right context read better than 500 that repeat. Revisit only if every
system in §2 is finished with a week spare.

---

## 7. Scope ladder — cut from the bottom

Decided **now**, in cold blood, so that week four is execution and not panic.

### Tier 0 — without these there is no entry
1. ~~Day cycle + End Day~~ `[BUILT]` · **upkeep still missing**
2. ~~Forge~~ `[BUILT]` — as a minigame, see §4.2
3. Guild scene + sponsorship
4. Drama tick + evening report
5. 60 drama events
6. Win + lose + ending screen

### Tier 1 — makes it a good entry
7. Reputation affecting which parties will deal with you
8. Party roster persisting across days, with names you remember
9. Forge fatigue and consumables that matter
10. **Tools affecting the QTE** — moved up from nothing; §4.2 argues it is now
    the cheapest way to stop six EQUIPMENT slots being furniture

### Tier 2 — cut without hesitation
11. Demon King finale as its own scene with art
12. Multiple endings
13. Ore Road / Capital Gate

### Tier 3 — cut first
14. Options menu (currently says so honestly)
15. Music and SFX — but see the note below
16. ~~Forge minigame~~ **built anyway**
17. ~~Ambient VFX~~ **built anyway** — the hearth, 1.9, ~2 hours
18. 500-event target

**Rule: nothing from Tier 1 starts until all of Tier 0 runs end to end**, even
badly. A rough complete loop beats a beautiful half.

**The rule was broken twice and it should be said plainly.** Item 16 sat in
Tier 3, "cut first", and was built while items 3–6 sat untouched. It came in
at about a third of its estimate and it is the best thing in the demo, so the
outcome was good and the process was not — the same bet at three days would
have cost a fifth of the remaining schedule for a system the ladder had
already ranked last. The ladder is only worth having if it survives a tempting
exception.

Item 17 is the second, and it is the defensible kind. Two hours is below the
threshold at which the ladder is the right tool: a rule that forbids a
two-hour improvement is a rule that will be ignored, and then it will be
ignored for a three-day one. **The line to hold is duration, not tier.**
Anything under half a day may jump the queue. Anything over it may not,
whatever tier it sits in and however good it would be.

**On item 15:** the QTE has a hit and a miss and no sound for either, and a
hit with no click reads as an input that did not register. This is the one
audio cue that is a *correctness* problem rather than polish. If any audio
ships, it is these two, and they are minutes rather than a day.

---

## 8. Schedule — 27 days

Rewritten from the 1.4 version, which had the forge occupying Aug 7–13. It is
done, so everything moves left and the Aug 20 gate is now reachable with slack
if the Guild is unblocked immediately.

| Window | Work | Gate |
|---|---|---|
| **Aug 8–9** | Upkeep. Guild backdrop commissioned/generated. Fix §9.6. | days cost money |
| **Aug 10–14** | Guild scene, party table, sponsorship flow | you can stake gear on a party |
| **Aug 15–19** | Drama tick in `AdvanceDay`, evening report in the balloon | **Tier 0 runs end to end** |
| **Aug 20–26** | 60 events, parties, recipes, promoted outputs, balance | a full run is winnable |
| **Aug 27–30** | Win/lose/ending. Tier 1 only if the gate passed. | — |
| **Aug 31–Sep 2** | Polish, bug pass, full playthroughs, QTE hit/miss sound | 3 clean runs |
| **Sep 3** | Size audit, strip, `config.h` module cull | under 1,474,560 |
| **Sep 4** | Submit **before 23:39** | done |

**The gate is now Aug 19.** If Tier 0 is not running end to end by then, cut
Tier 1 entirely and spend the remaining time on content and balance.

**The single highest-leverage action this week is Guild art**, because it is
the only Tier 0 blocker that cannot be resolved by writing C.

---

## 9. Decisions needed from Jack

Ordered by how much is blocked behind them. Struck items are resolved.

1. ~~**Forge: table roll or minigame?**~~ **RESOLVED — minigame**, built in
   1.6 at roughly a third of the estimate. §4.2 records the overrule.
2. **Drama event schema — approve or amend §4.4** before any event is
   written. **This is now the irreversible one and the most urgent.**
3. **Guild backdrop** — art, not a decision, but it blocks Tier 0 item 3 and
   nothing else can start it. See §8.
4. **Campaign length.** 30 days proposed. Drives all balance. The day counter
   exists and counts up forever; it needs an end.
5. **Daily upkeep formula.** `12 G + 2 G/party` proposed. Half a day's work,
   and until it lands the clock is decorative.
6. **Shop restock — and it got worse.** 1.7 added a restock at dawn *and left
   the one on room entry in place*, so the shelves now refill twice. The
   per-visit exploit — buy, leave, re-enter, buy again — is still live, and
   there is now a principled place to fix it. Recommendation: **delete the
   `ShopRestock` in `LoadRoom` and keep the one in `AdvanceDay`.** Stock
   becomes a daily resource, which is what a day cycle is for.
7. **Is party greed visible before committing?** Recommendation: **visible.**
8. **Nothing forgeable is above Rare**, and §4.5 gates the win on Legendary.
   Either a late recipe produces one, or the gate changes. Cheapest:
   a `[LOCKED]` late recipe with expensive ingredients and a long sequence.
9. **What happens to EQUIPMENT?** §4.2 lists three options.
   Recommendation: **tools widen the QTE window.**
10. **Evening report: balloon or its own screen?** Recommendation:
    **balloon**, and spend the saved day on events.
11. **Confirm the Tier 2/3 cuts in §7.** Especially: the Demon King finale may
    be a text ending rather than a scene.

---

## 10. Technical constraints — non-negotiable

- **1,474,560 bytes** decompressed. Currently **112,882 (7.66%)** as a
  measured `-O2` object, before static Raylib is linked. Projected with all of
  Tier 0 plus 80 events and one Guild backdrop: **~9.5%**. Size will not be
  what stops this project.
- Pure C11, static Raylib (`PLATFORM_DESKTOP_RGFW`), unity build.
- 320×240 internal, 4:3 pillarbox.
- Standalone executable. No browser build — contest rule 4.
- All assets embedded as DEFLATE'd indexed C arrays. No runtime file loading
  except the save.
- Every list-screen string budgeted against its panel at compile time.
- **In a unity build, file scope is not scope.** Macros *and* `static`
  functions collide across `.c` files. Every module prefixes both — `F_`/
  `Forge`, `Q_`/`Qte`. This has cost time twice; it is here so it does not
  cost it a third time.
- **A widget with a fixed size is a widget that will be overflowed**, and an
  enumerated test will not catch it. 1.7's END DAY note drew outside a
  152 px prompt box while a test that measured prompt strings passed, because
  it measured a hand-written list and the new string was not on it. Panels
  measure their contents; tests assert the invariant, not a list of the cases
  somebody remembered. This applies directly to the unbuilt **evening report**
  in §4.4, which is the next thing that will put unpredictable text on screen.
- **Effects are data on the room, not code in the room.** Lights are a table
  of up to four 12-byte `LightDef` on `SceneDef`; a room without any costs a
  branch. The Guild's brazier is a table row, and so is whatever the Guild
  turns out to have on its walls.
- **The brightest thing in a room is usually a window.** Locating the shop's
  lamps by luminance found the doorway and the skylight instead. Warmth over
  luminance, inside a region chosen by eye, is the method that works — and a
  *weighted centroid over the core*, not the single brightest cell, which is
  one sample and lands wherever the resampler put a highlight. Coordinates get
  pinned in a test to within a pixel, so a re-export cannot move a glow onto a
  shelf but a different resampler does not fail the build.
- **Tune an effect against what is behind it, not against itself.** The lamp
  flicker floored at 0.74 of its own peak, which moved the screen by three
  parts in a hundred. A parameter that is gentle in the effect's units and
  invisible in the frame's is a parameter nobody can review.
- **A visible version stamp means every change takes a number.** 1.9.2 exists
  because one coordinate moved. A build that differs from its stamp is a
  screenshot that lies about which build took it.
- **Anything on screen in every frame has one definition.** The version was in
  four places while it was only ever a comment. It is drawn now, so it lives
  in `src/version.h` and nothing else spells it out.
- **A second random generator is not duplication.** `vfx.c` has its own hash
  precisely so ambience cannot perturb the forge minigame's seeded sequence.
  Anything else that wants randomness gets its own too.

Full engineering rationale lives in `README.md`. **Where the two disagree,
this file is the intent and the README is the record.**

---

## 11. What changed since 1.4, and what it cost

Kept as a record so the same estimates are not made twice.

| | Estimated | Actual | Note |
|---|---|---|---|
| Forge + panel | 3 days | ~1 day | UI layer already existed |
| Forge minigame | 3 days (and cut) | ~1 day | rectangles, not art; 4 constants, not a curve |
| Day cycle | 1 day | ~0.5 day | reused the travel fade; **upkeep not done** |
| Menu unification | not planned | ~0.5 day | paid for by removing two auto-open paths |
| Prompt + HUD pass (1.8) | not planned | ~0.25 day | fixed an overflow a passing test had missed |
| Hearth VFX (1.9) | Tier 3, cut | ~2 hours | no assets; art measured, not eyeballed |
| Room lights + stamp (1.9.1) | Tier 3, cut | ~1 hour | the lamp case fell out of the hearth's data |

**Net: about 3 days spent, 4 days of ladder cleared, and one new `[OPEN]`
created** (EQUIPMENT losing its mechanic). The estimates in §2 for the
remaining systems are the *original* ones and have not been revised down —
the four that remain are the four with no existing scaffolding, which is
exactly why they were slow to start and why they will not come in at a third
of estimate.

**Size delta over the same window:** 97,047 → 112,806 bytes, +15,759 for the
forge, the minigame, the day cycle, the menu work, the 1.8 furniture and the
room lights. At that rate the remaining Tier 0 lands around 130 KB, or 8.8%.

**Four consecutive releases have been polish.** 1.7 was the last one that
moved a Tier 0 item, and that was the day cycle without its upkeep. The
duration rule in §7 is holding — nothing since has cost more than two hours —
but the rule was written to stop a three-day detour, not to license an
indefinite run of two-hour ones. §8's gate is Aug 19 and §9.3 is still art
that has not been started.
