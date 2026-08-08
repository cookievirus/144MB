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
#include "title.c"

void StubClearSave(void);
void StubCorruptSave(void);


static void settle(Scene *s) { for (int i = 0; i < 400; i++) SceneUpdate(s, 1.0f/60.0f); }

/* Walks a script to its end, one accept per frame, and stops as soon as the
   dialogue clears so the caller can see what the script handed control to. */
static void finish(Scene *s)
{
    for (int i = 0; i < 80 && s->dialog.phase != DIALOG_HIDDEN; i++) {
        SceneAdvance(s);
        SceneUpdate(s, 1.0f/60.0f);
    }
}

int main(void)
{
    /* ---- title: LOAD is not offered without a save --------------------- */
    StubClearSave();
    UiTitle t;
    TitleLoad(&t);
    printf("title: has_save %d, cursor %d\n", t.has_save, t.cursor);
    CHECK(!t.has_save, "no save at boot");
    CHECK(TitleAccept(&t) == TITLE_START, "row 0 is START");

    TitleMove(&t, 1);
    CHECK(t.cursor == 2, "DOWN steps over a dead LOAD row");
    CHECK(TitleAccept(&t) == TITLE_NONE, "OPTION does not start anything");
    CHECK(t.note != NULL, "OPTION says so instead of doing nothing");
    printf("OPTION note: \"%s\"\n", t.note);

    TitleMove(&t, 1);
    CHECK(TitleAccept(&t) == TITLE_EXIT, "row 3 is EXIT");
    TitleMove(&t, 1);
    CHECK(t.cursor == 0, "the menu wraps");
    TitleMove(&t, -1);
    CHECK(t.cursor == 3, "and wraps the other way");

    /* ---- a game, a save, and a resume ---------------------------------- */
    Scene sc;
    SceneInit(&sc, SCENE_SMITHY);
    settle(&sc);

    g_inv.gold = 1234;
    g_inv.held[ITM_COAL] = 7;
    g_inv.held[ITM_SILVER_INGOT] = 3;
    BeginTravel(&sc, SCENE_SHOP);
    settle(&sc);
    CHECK(sc.id == SCENE_SHOP, "in the shop");
    CHECK(WriteSave(&sc), "save written");

    TitleRefresh(&t);
    CHECK(t.has_save, "the title now offers LOAD");
    t.cursor = 1;
    CHECK(TitleAccept(&t) == TITLE_LOAD, "LOAD is selectable");

    /* Wreck the world, then load it back. */
    Scene ld;
    SceneInit(&ld, SCENE_SMITHY);
    CHECK(InvGold() == GOLD_START, "a new game reseeds the purse");
    CHECK(SceneLoadSave(&ld), "save loads");
    printf("\nloaded: room %d, gold %d, coal %d, silver %d\n",
           ld.id, InvGold(), InvHeld(ITM_COAL), InvHeld(ITM_SILVER_INGOT));
    CHECK(ld.id == SCENE_SHOP, "the saved room is restored");
    CHECK(InvGold() == 1234, "gold is restored");
    CHECK(InvHeld(ITM_COAL) == 7, "held counts are restored");
    CHECK(InvHeld(ITM_SILVER_INGOT) == 3, "and not just the first one");
    CHECK(ld.phase == PHASE_DONE, "a load resumes, it does not replay");
    CHECK(ld.dialog.phase == DIALOG_HIDDEN, "no welcome on a resume");
    CHECK(ld.actor_x[1] == (float)SCENES[SCENE_SHOP].actor[1].rest_x,
          "actors are already on their marks");

    StubCorruptSave();
    CHECK(!SceneSaveExists(), "a corrupt blob is refused, not guessed at");
    CHECK(!SceneLoadSave(&ld), "and loading it fails cleanly");
    StubClearSave();
    CHECK(!SceneSaveExists(), "a missing file is refused too");
    SceneUnload(&ld);

    /* ---- the shop conversation loop ------------------------------------ */
    Scene sh;
    SceneInit(&sh, SCENE_SHOP);
    settle(&sh);
    REQUIRE(sh.dialog.phase != DIALOG_HIDDEN, "the welcome plays");
    CHECK(!ShopIsOpen(&sh.shop), "and the counter waits for it");

    finish(&sh);
    /* 1.7 took the self-opening counter away: the welcome is a welcome, and
       the way in is the menu, everywhere, in every room. */
    CHECK(!ShopIsOpen(&sh.shop), "the welcome no longer opens it by itself");
    REQUIRE(DriveFeature(&sh), "the shop offers a BUY/SELL row");
    CHECK(ShopIsOpen(&sh.shop), "the menu row opens the counter");
    printf("\nmenu -> counter opened\n");

    /* Set up some state that "anything else" must not throw away. */
    ShopInput(&sh.shop, 1, 0, false);          /* SELL tab */
    ShopSort(&sh.shop, SORT_ALPHA);
    ShopInput(&sh.shop, 0, 2, false);
    const signed char tab = sh.shop.tab, cur = sh.shop.cursor;
    const unsigned char srt = sh.shop.sort.mode;

    SceneBack(&sh);
    CHECK(!ShopIsOpen(&sh.shop), "ESC closes the counter");
    REQUIRE(sh.dialog.phase != DIALOG_HIDDEN && sh.dialog.line != NULL,
            "and JACK speaks");
    CHECK(sh.dialog.line->set == PORTRAIT_MERCHANT, "it is JACK");
    CHECK(!UiPromptIsOpen(&sh.prompt), "the question comes after the line");
    printf("ESC -> \"%.36s...\"\n", sh.dialog.line->text);

    finish(&sh);
    REQUIRE(UiPromptIsOpen(&sh.prompt), "then the prompt");
    CHECK(sh.prompt.cursor == AGAIN_YES, "YES is the default");

    SceneAdvance(&sh);
    CHECK(ShopIsOpen(&sh.shop), "YES reopens the counter");
    CHECK(sh.shop.tab == tab && sh.shop.cursor == cur &&
          sh.shop.sort.mode == srt, "and keeps tab, cursor and sort");
    printf("YES -> resumed on tab %d, cursor %d, sort %d\n",
           sh.shop.tab, sh.shop.cursor, sh.shop.sort.mode);

    SceneBack(&sh);
    finish(&sh);
    CHECK(UiPromptIsOpen(&sh.prompt), "asked again");
    UiPromptMove(&sh.prompt, 1, 0);
    SceneAdvance(&sh);
    CHECK(!ShopIsOpen(&sh.shop), "NO leaves the counter shut");
    CHECK(sh.dialog.phase != DIALOG_HIDDEN, "with a parting line");
    finish(&sh);
    for (int i = 0; i < 120; i++) SceneUpdate(&sh, 1.0f/60.0f);
    CHECK(sh.id == SCENE_SMITHY, "NO walks out of the shop");
    CHECK(!SceneWantsQuit(&sh), "leaving the shop never quits the game");
    printf("NO -> back in room %d\n", sh.id);

    /* ---- prices carry their unit --------------------------------------- */
    {
        const int reserve_chars = ROW_MONEY_RESERVE / FONT_CELL_W;
        printf("\nmoney column reserves %d px = %d chars\n",
               ROW_MONEY_RESERVE, reserve_chars);
        CHECK(reserve_chars >= 6, "room for \"9999 G\"");

        /* The longest name any list can show must still leave the number
           column clear once it has been clipped. */
        int widest = 0;
        for (int i = 0; i < ITEM_COUNT; i++)
            if (UiTextWidth(ITEMS[i].name) > widest)
                widest = UiTextWidth(ITEMS[i].name);
        const int room = LIST_W - ROW_TEXT_X - ROW_MONEY_RESERVE;
        printf("longest item name %d px, row allows %d px%s\n", widest, room,
               widest > room ? " (clipped)" : "");
        CHECK(room > 0, "a row has room for a name at all");
        CHECK(LIST_X + ROW_TEXT_X + room + ROW_MONEY_RESERVE <= DETAIL_X - 4,
              "the money column clears the detail pane");
    }

    SceneUnload(&sh);
    SceneUnload(&sc);
    TitleUnload(&t);
    REPORT();
}
