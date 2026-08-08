/* test_day.c - what 1.7 changed: one way in through the main menu, a room
   feature row that follows the room, the day cycle, and the unit suffixes. */
#include "raylib.h"
#include <stdio.h>
#include <string.h>

#include "check.h"

void StubClearSave(void);   /* raylib_stub.c */

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

#define TICK (1.0f / 60.0f)

static void Settle(Scene *s, int frames)
{
    for (int i = 0; i < frames; i++) SceneUpdate(s, TICK);
}

static const char *RootName(const UiMenu *m, int row)
{
    return RootLabel(m, row);
}

int main(void)
{
    Scene sc;
    SceneInit(&sc, SCENE_SMITHY);
    Settle(&sc, 240);
    DriveSkipDialog(&sc);

    /* --- the root grid follows the room --------------------------------- */
    SceneToggleMenu(&sc);
    REQUIRE(UiMenuIsOpen(&sc.menu), "M opens the menu");

    unsigned char rows[ROOT_MAX_ROWS];
    int n = RootBuild(&sc.menu, rows);
    printf("smithy root (%d rows):", n);
    for (int i = 0; i < n; i++) printf(" %s;", RootName(&sc.menu, rows[i]));
    printf("\n");

    CHECK(n == 6, "a room with a feature shows six rows");
    CHECK(rows[0] == ROOT_TALK, "TALK is always first");
    CHECK(rows[1] == ROOT_FEATURE, "the room's feature is second");
    CHECK(rows[n - 1] == ROOT_END_DAY, "END DAY is always last");
    CHECK(strcmp(RootName(&sc.menu, ROOT_FEATURE), "FORGE") == 0,
          "the smithy's row is labelled FORGE");

    /* Every label has to fit its cell, and none of them is drawn anywhere
       that would reveal an overflow before a player did. */
    for (int i = 0; i < n; i++) {
        const char *label = RootName(&sc.menu, rows[i]);
        REQUIRE(label != NULL, "every listed row has a label");
        CHECK(UiTextWidth(label) + 5 <= CELL_W, "label fits its cell");
    }
    for (int fkind = 1; fkind < ROOM_FEATURE_COUNT; fkind++) {
        const char *label = ROOM_FEATURE_LABELS[fkind];
        REQUIRE(label != NULL, "every feature has a label");
        CHECK(UiTextWidth(label) + 5 <= CELL_W, "feature label fits its cell");
        printf("feature %d -> %s\n", fkind, label);
    }
    CHECK(ROOM_FEATURE_LABELS[ROOM_FEATURE_NONE] == NULL,
          "a room with no feature has no label to draw");

    /* --- the ragged last row cannot strand the cursor -------------------- */
    /* Six rows is a full 2x3 grid, so force the short case by hand: a room
       with no feature is five rows, and the sixth cell does not exist. */
    UiMenu bare;
    UiMenuInit(&bare);
    UiMenuOpen(&bare, ROOM_FEATURE_NONE);
    n = RootBuild(&bare, rows);
    CHECK(n == 5, "a featureless room shows five rows");
    CHECK(rows[1] == ROOT_INVENTORY, "and no feature row");

    UiMenuInput(&bare, 0, 1, false, false);
    UiMenuInput(&bare, 0, 1, false, false);      /* down to the short row */
    UiMenuInput(&bare, 1, 0, false, false);      /* right, into the gap   */
    printf("featureless grid: cursor parked at %d of %d\n",
           bare.stack[0].cursor, n);
    CHECK(bare.stack[0].cursor == n - 1, "the cursor clamps to the last row");

    /* --- the feature row opens the room's own thing ---------------------- */
    UiMenuClose(&sc.menu);
    REQUIRE(DriveFeature(&sc), "the smithy has a feature row");
    CHECK(UiPromptIsOpen(&sc.prompt), "FORGE raises the anvil prompt");
    UiPromptClose(&sc.prompt);

    /* --- nothing opens itself any more ----------------------------------- */
    Scene shop;
    SceneInit(&shop, SCENE_SHOP);
    Settle(&shop, 400);
    CHECK(!ShopIsOpen(&shop.shop), "the welcome does not open the counter");
    DriveSkipDialog(&shop);
    SceneAdvance(&shop);
    CHECK(!ShopIsOpen(&shop.shop), "nor does the accept key on an idle room");
    REQUIRE(DriveFeature(&shop), "the shop has a feature row");
    CHECK(ShopIsOpen(&shop.shop), "BUY/SELL is the only way to the counter");
    CHECK(strcmp(RootName(&shop.menu, ROOT_FEATURE), "BUY/SELL") == 0,
          "and it is labelled so");
    ShopClose(&shop.shop);
    SceneUnload(&shop);

    /* --- the day ---------------------------------------------------------- */
    CHECK(sc.day == 1, "a new game starts on day one");

    /* Spend some stock so the restock has something to prove. */
    REQUIRE(DriveFeature(&sc), "back to the anvil");
    UiPromptClose(&sc.prompt);
    sc.shop.stock[0] = 0;

    REQUIRE(DriveRoot(&sc, ROOT_END_DAY), "END DAY is on the grid");
    REQUIRE(UiPromptIsOpen(&sc.prompt), "and it asks first");
    CHECK(sc.prompt.cursor == END_DAY_NO, "with NOT YET as the default");
    CHECK(UiMenuIsOpen(&sc.menu), "the menu stays open behind the question");
    printf("\nend-day prompt: \"%s\" / \"%s\"\n",
           sc.prompt.options[0], sc.prompt.options[1]);

    /* Saying no changes nothing. */
    SceneAdvance(&sc);
    CHECK(!UiPromptIsOpen(&sc.prompt), "NOT YET closes the question");
    CHECK(sc.day == 1, "and does not end the day");
    CHECK(sc.fade_dir == 0, "and starts no fade");
    CHECK(UiMenuIsOpen(&sc.menu), "and leaves the menu where it was");

    /* Saying yes does. */
    REQUIRE(DriveRoot(&sc, ROOT_END_DAY), "END DAY again");
    UiPromptMove(&sc.prompt, 1, 0);
    CHECK(sc.prompt.cursor == END_DAY_YES, "moved onto END DAY");
    SceneAdvance(&sc);
    CHECK(sc.fade_dir > 0, "the day turns over behind a fade");
    CHECK(sc.fade_kind == FADE_DAY, "and the fade knows what it is hiding");
    CHECK(!UiMenuIsOpen(&sc.menu), "the menu closed");
    CHECK(sc.day == 1, "the date does not change until full black");

    const unsigned char room_before = sc.id;
    Settle(&sc, 120);
    printf("after the fade: day %d, scene %d, fade %.2f\n",
           (int)sc.day, sc.id, (double)sc.fade);
    CHECK(sc.day == 2, "morning");
    CHECK(sc.id == room_before, "a day boundary does not move the player");
    CHECK(sc.fade_dir == 0 && sc.fade == 0.0f, "the fade resolved");
    CHECK(sc.shop.stock[0] == SHOP_STOCK[0].stock, "the shelves refilled");
    CHECK(sc.dialog.phase != DIALOG_HIDDEN, "and the day is announced");
    CHECK(sc.dialog.line->speaker == NULL, "by nobody in particular");

    /* --- the day survives a save ---------------------------------------- */
    DriveSkipDialog(&sc);
    StubClearSave();
    sc.prompt_kind = PROMPT_SYSTEM;
    UiPromptOpen(&sc.prompt, "PAUSED", SYSTEM_OPTIONS, 3, SYS_SAVE);
    SceneAdvance(&sc);
    UiPromptClose(&sc.prompt);

    sc.day = 99;
    REQUIRE(SceneLoadSave(&sc), "the save round-trips");
    CHECK(sc.day == 2, "and it remembers which day it was");
    printf("save blob %d bytes, version %d\n", (int)sizeof(SaveBlob), SAVE_VERSION);

    SceneReset(&sc);
    CHECK(sc.day == 1, "a new game goes back to day one");

    /* --- the standing HUD ------------------------------------------------ */
    DriveSkipDialog(&sc);
    Settle(&sc, 300);
    DriveSkipDialog(&sc);
    CHECK(RoomIsIdle(&sc), "an empty room is idle, so the hint shows");
    SceneToggleMenu(&sc);
    CHECK(!RoomIsIdle(&sc), "the hint hides once the menu is up");
    UiMenuClose(&sc.menu);

    /* 1.8: the badge is a bordered panel now, so it no longer fits entirely
       in the 8 px margin - it sits on the page frame's top edge. What still
       has to hold is that it clears the title line, because that is where the
       shop draws its purse, and the two would overlap exactly. */
    CHECK(HUD_DAY_Y + HUD_DAY_H <= PAGE_Y + 7,
          "the date badge clears every page's title line");
    CHECK(HUD_DAY_X + HUD_DAY_W < VSCREEN_W, "and stays on screen");
    CHECK(HUD_DAY_PAD * 2 + FONT_CELL_W * (3 + HUD_DAY_DIGITS) <= HUD_DAY_W,
          "with room for DAY and five digits");
    printf("date badge %dx%d at (%d,%d)\n",
           HUD_DAY_W, HUD_DAY_H, HUD_DAY_X, HUD_DAY_Y);
    CHECK(UiTextWidth(HUD_HINT) + HUD_HINT_X < VSCREEN_W, "the hint fits");
    CHECK(HUD_HINT_Y + 7 <= VSCREEN_H, "and sits on screen");

    /* --- units ------------------------------------------------------------ */
    /* "9999 EA" and "9999 G" have to fit the column reserved for them, or the
       row label runs into the number - which is exactly the class of bug the
       hint assertions were added for in 1.3. */
    CHECK(UiTextWidth("65535 EA") <= ROW_COUNT_RESERVE,
          "the widest count the field can hold fits its column");
    CHECK(UiTextWidth("9999 G") <= ROW_MONEY_RESERVE,
          "a four-digit price fits its column");
    CHECK(ROW_COUNT_RESERVE > ROW_QTY_RESERVE,
          "the unit costs the label characters, and is budgeted for");
    CHECK(LIST_W - ROW_TEXT_X - ROW_COUNT_RESERVE >= FONT_CELL_W * 12,
          "and still leaves twelve characters of label");
    printf("reserves: qty %d, money %d, count %d px\n",
           ROW_QTY_RESERVE, ROW_MONEY_RESERVE, ROW_COUNT_RESERVE);

    /* --- the pause box ---------------------------------------------------- */
    /* 1.8 reordered and stacked it: SAVE, CANCEL, QUIT, reading in the order
       the actions escalate, with the safe one under the cursor. */
    OpenSystemPrompt(&sc);
    REQUIRE(UiPromptIsOpen(&sc.prompt), "ESC raises the pause box");
    CHECK(sc.prompt.layout == PROMPT_COLUMN, "stacked, not in a row");
    CHECK(sc.prompt.count == 3, "three choices");
    CHECK(strcmp(sc.prompt.options[0], "SAVE") == 0, "SAVE reads first");
    CHECK(strcmp(sc.prompt.options[1], "CANCEL") == 0, "CANCEL in the middle");
    CHECK(strcmp(sc.prompt.options[2], "QUIT") == 0, "QUIT last");
    CHECK(sc.prompt.cursor == SYS_CANCEL, "and the safe one is under the cursor");
    CHECK(SYS_SAVE == 0 && SYS_CANCEL == 1 && SYS_QUIT == 2,
          "the constants follow the labels");

    /* A column steers on the vertical axis and ignores the other one, so a
       nudge sideways cannot silently move the player off CANCEL onto QUIT. */
    UiPromptMove(&sc.prompt, 1, 0);
    CHECK(sc.prompt.cursor == SYS_CANCEL, "left and right do nothing to a column");
    UiPromptMove(&sc.prompt, 0, 1);
    CHECK(sc.prompt.cursor == SYS_QUIT, "down reaches QUIT");
    UiPromptMove(&sc.prompt, 0, 1);
    CHECK(sc.prompt.cursor == SYS_SAVE, "and wraps to the top");
    UiPromptMove(&sc.prompt, 0, -1);
    CHECK(sc.prompt.cursor == SYS_QUIT, "up wraps the other way");
    UiPromptClose(&sc.prompt);

    /* The row prompts keep the other axis, and ignore the vertical. */
    OpenEndDayPrompt(&sc);
    CHECK(sc.prompt.layout == PROMPT_ROW, "a two-answer question stays a row");
    UiPromptMove(&sc.prompt, 0, 1);
    CHECK(sc.prompt.cursor == END_DAY_NO, "up and down do nothing to a row");
    UiPromptMove(&sc.prompt, 1, 0);
    CHECK(sc.prompt.cursor == END_DAY_YES, "right moves along it");
    UiPromptClose(&sc.prompt);

    SceneUnload(&sc);
    REPORT();
}
