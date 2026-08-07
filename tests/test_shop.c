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
#include "ui_menu.c"
#include "ui_prompt.c"
#include "ui_dialog.c"
#include "scene.c"


static void step(Scene *s, int n) { for (int i=0;i<n;i++) SceneUpdate(s, 1.0f/60.0f); }

int main(void)
{
    Scene sc;
    SceneInit(&sc, SCENE_SMITHY);

    printf("gold start        %d\n", InvGold());
    CHECK(InvGold() == GOLD_START, "gold seeded");
    CHECK(InvHeld(ITM_COAL) == 112, "held seeded from ITEMS");

    /* --- pricing ------------------------------------------------------- */
    printf("\nrow  item              tier  base  buy  sell\n");
    for (int i = 0; i < STOCK_COUNT; i++) {
        const int it = SHOP_STOCK[i].item;
        printf("%2d   %-18s %-9s %3d  %4d  %4d\n", i, ITEMS[it].name,
               RarityName((Rarity)ITEMS[it].rarity), SHOP_STOCK[i].base,
               InvBuyPrice(i), InvSellPrice(it));
        CHECK(InvBuyPrice(i) >= 1, "buy price never free");
        CHECK(InvSellPrice(it) < InvBuyPrice(i) || InvBuyPrice(i) <= 2,
              "spread favours the trader");
    }

    /* --- buy / sell ----------------------------------------------------- */
    ShopOpen(&sc.shop);
    sc.shop.tab = SHOP_BUY; sc.shop.cursor = 0;   /* Small Potion */
    const int before_gold = InvGold();
    const int before_held = InvHeld(ITM_SMALL_POTION);
    const int price = InvBuyPrice(0);
    ShopInput(&sc.shop, 0, 0, true);
    printf("\nbuy potion: gold %d -> %d (price %d), held %d -> %d\n",
           before_gold, InvGold(), price, before_held, InvHeld(ITM_SMALL_POTION));
    CHECK(InvGold() == before_gold - price, "gold debited exactly");
    CHECK(InvHeld(ITM_SMALL_POTION) == before_held + 1, "one unit added");
    CHECK(sc.shop.stock[0] == SHOP_STOCK[0].stock - 1, "stock decremented");

    /* Drain the row and confirm it locks. Gold is topped up first: the coin
       check would otherwise stop the loop before the stock check ever fires,
       which is correct behaviour but tests the wrong guard. */
    g_inv.gold = 100000;
    for (int i = 0; i < 50; i++) ShopInput(&sc.shop, 0, 0, true);
    CHECK(sc.shop.stock[0] == 0, "row sells out");
    CHECK(InvHeld(ITM_SMALL_POTION) == before_held + SHOP_STOCK[0].stock,
          "exactly the stock changed hands");
    g_inv.gold = 250;
    CHECK(InvGold() >= 0, "gold never negative");
    printf("after draining row 0: stock %d, gold %d, result \"%s\"\n",
           sc.shop.stock[0], InvGold(), sc.shop.result ? sc.shop.result : "");

    /* Spend to empty and confirm the coin check holds. */
    sc.shop.cursor = 13;                       /* Chain Coif, dearest row */
    int guard = 0;
    while (InvGold() >= InvBuyPrice(13) && guard++ < 100) {
        ShopInput(&sc.shop, 0, 0, true);
    }
    ShopInput(&sc.shop, 0, 0, true);
    CHECK(InvGold() >= 0, "cannot overdraw");
    printf("after overspend attempt: gold %d, result \"%s\"\n",
           InvGold(), sc.shop.result ? sc.shop.result : "");

    /* --- sell tab, including the shrinking-list case -------------------- */
    ShopInput(&sc.shop, 1, 0, false);          /* -> SELL */
    CHECK(sc.shop.tab == SHOP_SELL, "tab switched");
    int rows = 0; for (int i=0;i<ITEM_COUNT;i++) if (InvHeld(i)>0) rows++;
    printf("\nsell rows: %d\n", rows);

    /* Sell the last unit of something and make sure the cursor survives. */
    sc.shop.cursor = (signed char)(rows - 1);
    guard = 0;
    while (rows > 1 && guard++ < 500) {
        ShopInput(&sc.shop, 0, 0, true);
        int now = 0; for (int i=0;i<ITEM_COUNT;i++) if (InvHeld(i)>0) now++;
        CHECK(sc.shop.cursor < now || now == 0, "cursor stays in range");
        if (now == rows - 1) break;
        rows = now;
    }
    printf("cursor %d after list shrank, gold %d\n", sc.shop.cursor, InvGold());
    ShopClose(&sc.shop);

    /* --- travel --------------------------------------------------------- */
    printf("\nscene %d -> travel\n", sc.id);
    UiMenuOpen(&sc.menu);
    UiMenuInput(&sc.menu, 0, 1, false, false);       /* root: down to MAP row */
    UiMenuInput(&sc.menu, 1, 0, false, false);
    MenuCommand cmd = UiMenuInput(&sc.menu, 0, 0, true, false);
    CHECK(cmd == MENU_CMD_NONE, "root accept pushes a screen");
    UiMenuInput(&sc.menu, 0, 1, false, false);       /* MAP: Market Row */
    cmd = UiMenuInput(&sc.menu, 0, 0, true, false);
    printf("map accept -> cmd %d (travel=%d scene=%d)\n",
           cmd, MenuCmdIsTravel(cmd), MenuCmdIsTravel(cmd) ? MenuCmdScene(cmd) : -1);
    CHECK(MenuCmdIsTravel(cmd), "Market Row returns a travel command");
    CHECK(MenuCmdScene(cmd) == SCENE_SHOP, "Market Row maps to the shop");

    BeginTravel(&sc, MenuCmdScene(cmd));
    step(&sc, 60);
    printf("after fade: scene %d, fade %.2f, dir %d, phase %d\n",
           sc.id, (double)sc.fade, sc.fade_dir, sc.phase);
    CHECK(sc.id == SCENE_SHOP, "landed in the shop");
    CHECK(sc.fade_dir == 0 && sc.fade == 0.0f, "fade resolved");
    /* The shop is no longer a static room: BEST walks in and JACK follows
       2.5 s later, so one second after the fade it is still entering. */
    CHECK(sc.phase == PHASE_ENTER, "shop entrance is still running at t=1s");
    step(&sc, 240);
    CHECK(sc.phase != PHASE_ENTER, "shop settles by t=5s");
    CHECK(sc.shop.stock[0] == SHOP_STOCK[0].stock, "restocked on entry");

    /* Closed destinations must not travel. */
    UiMenuOpen(&sc.menu);
    UiMenuInput(&sc.menu, 0, 1, false, false);
    UiMenuInput(&sc.menu, 1, 0, false, false);
    UiMenuInput(&sc.menu, 0, 0, true, false);
    UiMenuInput(&sc.menu, 0, 1, false, false);
    UiMenuInput(&sc.menu, 0, 1, false, false);       /* Adventurers Guild */
    cmd = UiMenuInput(&sc.menu, 0, 0, true, false);
    CHECK(!MenuCmdIsTravel(cmd), "unbuilt destination refuses travel");
    UiMenuClose(&sc.menu);

    /* --- ESC ladder ----------------------------------------------------- */
    /* 1.4: ESC at the counter closes it and hands the question to JACK. */
    ShopOpen(&sc.shop);
    SceneBack(&sc);
    CHECK(!ShopIsOpen(&sc.shop), "ESC closes the counter");
    CHECK(sc.dialog.phase != DIALOG_HIDDEN, "and JACK asks");
    UiDialogHide(&sc.dialog);
    UiPromptClose(&sc.prompt);
    UiMenuOpen(&sc.menu);
    SceneBack(&sc); CHECK(!UiMenuIsOpen(&sc.menu), "ESC closes the menu");
    UiDialogHide(&sc.dialog);
    SceneBack(&sc); CHECK(UiPromptIsOpen(&sc.prompt), "ESC raises the prompt");
    SceneBack(&sc); CHECK(!UiPromptIsOpen(&sc.prompt), "ESC closes the prompt");
    CHECK(!SceneWantsQuit(&sc), "ESC never quits by itself");

    /* --- travel back ---------------------------------------------------- */
    BeginTravel(&sc, SCENE_SMITHY);
    step(&sc, 60);
    CHECK(sc.id == SCENE_SMITHY, "travelled home");
    step(&sc, 200);
    CHECK(sc.phase == PHASE_SETTLED || sc.phase == PHASE_DONE, "hero entrance ran");
    printf("home: phase %d actor_x %.0f\n", sc.phase, (double)sc.actor_x[0]);

    printf("\nsave blob %d bytes\n", (int)sizeof(SaveBlob));
    SceneUnload(&sc);

    REPORT();
}
