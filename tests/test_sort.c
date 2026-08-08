#include "raylib.h"
#include <stdio.h>

#include "check.h"
#include "gfx.c"
#include "rarity.c"
#include "ui_font.c"
#include "ui.c"
#include "sort.c"
#include "inventory.c"
#include "shop.c"
#include "vfx.c"
#include "qte.c"
#include "forge.c"
#include "ui_menu.c"
#include "ui_prompt.c"
#include "ui_dialog.c"
#include "scene.c"

#include "drive.h"

static void step(Scene *s,int n){ for(int i=0;i<n;i++) SceneUpdate(s,1.0f/60.0f); }

static void dump(const char *tag, int n)
{
    const SortRow *r = SortScratch();
    printf("  %-12s ", tag);
    for (int i = 0; i < n && i < 6; i++) printf("%s(%d/%d) ", r[i].name, r[i].rarity, r[i].qty);
    printf("\n");
}

int main(void)
{
    Scene sc; SceneInit(&sc, SCENE_SHOP);
    step(&sc, 400);

    /* ---- sort ---------------------------------------------------------- */
    ShopOpen(&sc.shop);
    int n = BuildRows(&sc.shop);
    printf("BUY tab, %d rows\n", n);
    dump("default", n);

    ShopSort(&sc.shop, SORT_RARITY); n = BuildRows(&sc.shop); dump("R once", n);
    const SortRow *r = SortScratch();
    for (int i = 1; i < n; i++)
        CHECK(r[i-1].rarity >= r[i].rarity, "R once = rarity high to low");
    CHECK(sc.shop.cursor == 0, "sorting resets the cursor");

    ShopSort(&sc.shop, SORT_RARITY); n = BuildRows(&sc.shop); dump("R twice", n);
    r = SortScratch();
    for (int i = 1; i < n; i++)
        CHECK(r[i-1].rarity <= r[i].rarity, "R twice = rarity low to high");

    ShopSort(&sc.shop, SORT_QTY); n = BuildRows(&sc.shop); dump("T once", n);
    r = SortScratch();
    for (int i = 1; i < n; i++) CHECK(r[i-1].qty >= r[i].qty, "T once = most first");
    ShopSort(&sc.shop, SORT_QTY); n = BuildRows(&sc.shop); dump("T twice", n);
    r = SortScratch();
    for (int i = 1; i < n; i++) CHECK(r[i-1].qty <= r[i].qty, "T twice = least first");

    ShopSort(&sc.shop, SORT_ALPHA); n = BuildRows(&sc.shop); dump("A once", n);
    r = SortScratch();
    for (int i = 1; i < n; i++)
        CHECK(NameCmp(r[i-1].name, r[i].name) <= 0, "A once = A to Z");
    ShopSort(&sc.shop, SORT_ALPHA); n = BuildRows(&sc.shop); dump("A twice", n);
    r = SortScratch();
    for (int i = 1; i < n; i++)
        CHECK(NameCmp(r[i-1].name, r[i].name) >= 0, "A twice = Z to A");

    /* Ties must be alphabetical, not table order. */
    ShopSort(&sc.shop, SORT_RARITY); n = BuildRows(&sc.shop);
    r = SortScratch();
    for (int i = 1; i < n; i++)
        if (r[i-1].rarity == r[i].rarity)
            CHECK(NameCmp(r[i-1].name, r[i].name) <= 0, "ties break alphabetically");

    /* ---- buying the right thing under a sort --------------------------- */
    ShopSort(&sc.shop, SORT_ALPHA);
    n = BuildRows(&sc.shop);
    sc.shop.cursor = 3;
    int item = RowItem(&sc.shop, 3), st = RowStock(&sc.shop, 3);
    int held0 = InvHeld(item), stock0 = sc.shop.stock[st], gold0 = InvGold();
    int price = RowPrice(&sc.shop, 3);
    ShopInput(&sc.shop, 0, 0, true);
    printf("\nsorted buy: row 3 = %s (item %d, stock row %d)\n", ITEMS[item].name, item, st);
    CHECK(InvHeld(item) == held0 + 1, "the item under the cursor is what arrives");
    CHECK(sc.shop.stock[st] == stock0 - 1, "the matching stock row decrements");
    CHECK(InvGold() == gold0 - price, "gold matches the displayed price");

    /* ---- ESC at the counter hands the question to JACK -----------------
       1.4 replaced 1.3's END TRADING? modal: the counter closes, JACK asks
       out loud, and the prompt only collects the answer. The full loop is
       covered in test_title.c; what matters here is that the sort survives
       a trip through it. */
    const unsigned char kept = sc.shop.sort.mode;
    SceneBack(&sc);
    CHECK(!ShopIsOpen(&sc.shop), "ESC closes the counter");
    REQUIRE(sc.dialog.phase != DIALOG_HIDDEN, "and JACK speaks");
    for (int i = 0; i < 60 && sc.dialog.phase != DIALOG_HIDDEN; i++) {
        SceneAdvance(&sc); SceneUpdate(&sc, 1.0f/60.0f);
    }
    CHECK(UiPromptIsOpen(&sc.prompt), "then the prompt");
    SceneAdvance(&sc);                     /* YES */
    CHECK(ShopIsOpen(&sc.shop), "YES resumes the counter");
    CHECK(sc.shop.sort.mode == kept, "and the sort order survived");

    /* ---- M -> TALK -> line -> counter ----------------------------------
       The counter has to be shut first: M is refused while it is open, which
       is correct behaviour and was silently turning this whole block into a
       null dereference. */
    ShopClose(&sc.shop);
    UiDialogHide(&sc.dialog);
    UiPromptClose(&sc.prompt);

    SceneToggleMenu(&sc);
    REQUIRE(UiMenuIsOpen(&sc.menu), "M opens the menu in the shop");
    SceneAdvance(&sc);                       /* TALK is cursor 0 */
    CHECK(!UiMenuIsOpen(&sc.menu), "TALK closes the menu");
    REQUIRE(sc.dialog.phase != DIALOG_HIDDEN && sc.dialog.line != NULL,
            "TALK plays a line");
    CHECK(sc.dialog.line->set == PORTRAIT_MERCHANT, "JACK is speaking");
    CHECK(!ShopIsOpen(&sc.shop), "TALK does not open the counter");
    printf("\nTALK line 1: \"%.34s...\"\n", sc.dialog.line->text);
    CHECK(sc.dialog.line->text != SHOP_INTRO[0].text, "not the welcome again");

    /* 1.7: TALK is only talk. The counter is BUY/SELL, one row down, and
       when the conversation ends the room is idle rather than trading. */
    DriveSkipDialog(&sc);
    CHECK(!ShopIsOpen(&sc.shop), "and still does not when he finishes");
    REQUIRE(DriveFeature(&sc), "the shop offers a BUY/SELL row");
    CHECK(ShopIsOpen(&sc.shop), "which is what opens the counter");
    printf("counter opened after the TALK script\n");

    /* ---- sort keys route to the top layer only ------------------------- */
    ShopSort(&sc.shop, SORT_RARITY);
    const unsigned char before = sc.menu.sort.mode;
    SceneSort(&sc, SORT_ALPHA);
    CHECK(sc.shop.sort.mode == SORT_ALPHA, "sort reaches the shop");
    CHECK(sc.menu.sort.mode == before, "and not the menu behind it");
    /* A modal prompt is the top layer, so nothing behind it reorders. */
    SceneBack(&sc);
    for (int i = 0; i < 60 && !UiPromptIsOpen(&sc.prompt); i++) {
        SceneAdvance(&sc); SceneUpdate(&sc, 1.0f/60.0f);
    }
    SceneSort(&sc, SORT_QTY);
    CHECK(sc.shop.sort.mode == SORT_ALPHA, "a modal prompt blocks sorting");
    SceneBack(&sc); ShopClose(&sc.shop); UiDialogHide(&sc.dialog);

    /* ---- JACK's new timing --------------------------------------------- */
    Scene t; SceneInit(&t, SCENE_SHOP);
    const SceneDef *d = &SCENES[SCENE_SHOP];
    printf("\nJACK delay %.2fs, room settles at %.2fs\n",
           (double)d->actor[1].delay, (double)SettleTime(d));
    CHECK(d->actor[1].delay > 1.25f && d->actor[1].delay < 1.35f, "JACK delay is 1.3s");
    for (int f = 0; f < 300 && t.phase == PHASE_ENTER; f++) SceneUpdate(&t, 1.0f/60.0f);
    CHECK(t.actor_x[1] == (float)d->actor[1].rest_x, "JACK reaches his mark");
    SceneUnload(&t);

    /* ---- every drawn string fits its panel ----------------------------
       The compile-time guards cover the hint macros and the sort tags, but
       nothing checks the widths that are only known at run time: page titles
       and the purse. Measured here rather than eyeballed, because 1.3 shipped
       a hint line running twenty pixels past the frame. */
    {
        /* Page titles share the title line with the sort tag. Prompt titles
           do not - the prompt is a centred box with its own frame - so the
           two are measured against different things. */
        static const char *const TITLES[] = {
            "INVENTORY", "EQUIPMENT", "MAP", "ITEM SHOP"
        };
        for (int i = 0; i < (int)(sizeof(TITLES)/sizeof(TITLES[0])); i++)
            CHECK(LIST_X + UiTextWidth(TITLES[i]) < SORT_TAG_X,
                  "page title clears the sort tag");

        static const char *const TAGS[] = {
            "DEFAULT", "RARITY HIGH", "RARITY LOW", "QTY MOST",
            "QTY LEAST", "NAME A-Z", "NAME Z-A"
        };
        int widest = 0;
        for (int i = 0; i < (int)(sizeof(TAGS)/sizeof(TAGS[0])); i++)
            if (UiTextWidth(TAGS[i]) > widest) widest = UiTextWidth(TAGS[i]);
        printf("\nwidest sort tag %d px, ends at x=%d\n",
               widest, SORT_TAG_X + widest);

        /* Six digits of gold is more than the demo can reach, which is the
           point: the layout must survive the economy growing into it. */
        const int purse_left = (PAGE_X + PAGE_W - 14) - 6 * FONT_CELL_W;
        CHECK(SORT_TAG_X + widest < purse_left, "sort tag clears a 6-digit purse");
        printf("purse at 6 digits starts x=%d, clearance %d px\n",
               purse_left, purse_left - (SORT_TAG_X + widest));

        /* The prompt box used to be a fixed 152 px and this block used to be
           a list of the strings that went in it. That is why 1.7's END DAY
           note overflowed with a passing test suite: the check knew about the
           strings someone had remembered to add to it, and the new one was
           not among them.

           1.8 made the box measure its contents, so the thing to assert is
           the invariant - whatever is put in, the box grows to hold it and
           stays on screen - and then to check the real prompts against it
           rather than a copy of their labels. */
        UiPrompt pr;

        static const char *const SYS[] = { "SAVE", "CANCEL", "QUIT" };
        static const char *const TRD[] = { "KEEP TRADING", "DONE" };
        static const char *const END[] = { "NOT YET", "END DAY" };
        static const char *const SMI[] = { "FORGE", "BLUEPRINTS" };

        struct { const char *title; const char *const *opts; int n;
                 const char *note; bool column; } BOXES[] = {
            { "PAUSED",         SYS, 3, NULL,                                true  },
            { "PAUSED",         SYS, 3, "Progress saved.",                   true  },
            { "ANYTHING ELSE?", TRD, 2, NULL,                                false },
            { "END THE DAY?",   END, 2, "The forge goes cold until morning.", false },
            { "THE ANVIL",      SMI, 2, NULL,                                false },
        };

        for (int i = 0; i < (int)(sizeof(BOXES) / sizeof(BOXES[0])); i++) {
            if (BOXES[i].column) {
                UiPromptOpenColumn(&pr, BOXES[i].title, BOXES[i].opts,
                                   BOXES[i].n, 0);
            } else {
                UiPromptOpen(&pr, BOXES[i].title, BOXES[i].opts, BOXES[i].n, 0);
            }
            pr.note = BOXES[i].note;

            const int bw = UiPromptWidth(&pr);
            const int bh = UiPromptHeight(&pr);
            const int inner = bw - PROMPT_PAD * 2;

            printf("box \"%s\" %dx%d, inner %d\n", BOXES[i].title, bw, bh, inner);

            CHECK(bw <= PROMPT_MAX_W, "the box stays on screen");
            CHECK(bh <= VSCREEN_H, "and inside the frame vertically");
            CHECK(bw >= PROMPT_MIN_W, "and never shrinks below the minimum");
            CHECK(UiTextWidth(BOXES[i].title) <= inner, "title inside the box");
            if (BOXES[i].note != NULL) {
                CHECK(UiTextWidth(BOXES[i].note) <= inner, "note inside the box");
            }

            int row = 0, widest = 0;
            for (int b = 0; b < BOXES[i].n; b++) {
                const int w = UiTextWidth(BOXES[i].opts[b]) + 10;
                row += w + (b + 1 < BOXES[i].n ? 4 : 0);
                if (w > widest) widest = w;
            }
            CHECK((BOXES[i].column ? widest : row) <= inner,
                  "buttons inside the box");
        }

        /* And the invariant itself: a note far longer than anything shipped
           still ends up inside its own walls. */
        UiPromptOpen(&pr, "?", TRD, 2, 0);
        pr.note = "A note nobody has written yet, of some length.";
        CHECK(UiTextWidth(pr.note) <= UiPromptWidth(&pr) - PROMPT_PAD * 2,
              "an unforeseen note still fits, because the box follows it");

        static const char *const HINTS[] = {
            "L/R TAB  SPACE TRADE  R/T/A SORT  ESC LEAVE",
            "ARROWS MOVE   SPACE/ENTER SELECT   ESC BACK",
            "R RARITY  T QTY  A NAME   ESC BACK",
            "ARROWS MOVE   SPACE/ENTER TRAVEL   ESC BACK"
        };
        for (int i = 0; i < (int)(sizeof(HINTS)/sizeof(HINTS[0])); i++) {
            const int end = LIST_X + UiTextWidth(HINTS[i]);
            CHECK(end <= PAGE_X + PAGE_W - 4, "hint line fits inside the panel");
            printf("  hint %d ends at x=%d (panel edge %d)\n",
                   i, end, PAGE_X + PAGE_W);
        }
    }

    SceneUnload(&sc);
    REPORT();
}
