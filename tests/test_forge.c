/* test_forge.c - the anvil: gating, the consume/grant ledger, the sorted-row
   indirection, and the read-only guarantee on BLUEPRINTS. */
#include "raylib.h"
#include <stdio.h>

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

static int RecipeOf(const char *name)
{
    for (int i = 0; i < RECIPE_COUNT; i++) {
        if (ITEMS[RECIPES[i].out].name == name) return i;
    }
    return -1;
}

/* How many rows the screen is showing right now. */
static int Rows(const UiForge *f) { return ForgeBuildRows(f); }

int main(void)
{
    Scene sc;
    SceneInit(&sc, SCENE_SMITHY);

    /* --- table sanity --------------------------------------------------- */
    printf("recipe            shelf    out tier    ingredients\n");
    for (int i = 0; i < RECIPE_COUNT; i++) {
        const RecipeDef *r = &RECIPES[i];
        printf("%-17s %-8s %-9s", ITEMS[r->out].name, FORGE_CAT_NAMES[r->cat],
               RarityName((Rarity)ITEMS[r->out].rarity));
        for (int m = 0; m < RECIPE_SLOTS; m++) {
            if (r->mat[m].item == RECIPE_NONE) continue;
            printf(" %s x%d;", ITEMS[r->mat[m].item].name, r->mat[m].qty);
            CHECK(r->mat[m].qty > 0, "no zero-count ingredient");
            CHECK(r->mat[m].item < ITEM_COUNT, "ingredient is a real item");
        }
        printf("\n");
        CHECK(r->out < ITEM_COUNT, "output is a real item");
        CHECK(r->cat < FORGE_CAT_COUNT, "recipe sits on a real shelf");
        CHECK(InvKnows(i), "every shipped recipe starts known");
    }

    /* --- the shelf gates on materials, and does not hide the row -------- */
    UiForge *f = &sc.forge;
    ForgeOpen(f, FORGE_MODE_CRAFT);
    f->tab = FORGE_ARMOR;

    const int cuirass = RecipeOf("Iron Cuirass");
    REQUIRE(cuirass >= 0, "Iron Cuirass recipe exists");

    printf("\niron ore held %d, cuirass wants %d\n",
           InvHeld(ITM_IRON_ORE), RECIPES[cuirass].mat[0].qty);
    CHECK(!InvCanForge(cuirass), "cuirass is short at the seeded inventory");

    int armour_rows = Rows(f);
    bool cuirass_listed = false;
    for (int i = 0; i < armour_rows; i++) {
        if ((int)SortScratch()[i].ref == cuirass) cuirass_listed = true;
    }
    CHECK(armour_rows == 3, "armour shelf shows all three rows");
    CHECK(cuirass_listed, "an unaffordable recipe stays on the shelf");

    /* Selecting it and pressing accept must change nothing at all. */
    const int ore_before = InvHeld(ITM_IRON_ORE);
    const int made_before = InvHeld(ITM_CUIRASS);
    for (int i = 0; i < armour_rows; i++) {
        if ((int)SortScratch()[i].ref == cuirass) f->cursor = (signed char)i;
    }
    CHECK(ForgeInput(f, 0, 0, true) == FORGE_CMD_NONE,
          "an unaffordable row does not start a heat");
    CHECK(InvHeld(ITM_IRON_ORE) == ore_before, "a refused forge spends nothing");
    CHECK(InvHeld(ITM_CUIRASS) == made_before, "a refused forge makes nothing");

    /* --- a good forge debits exactly, once ------------------------------ */
    f->tab = FORGE_WEAPON;
    SortReset(&f->sort);
    const int dagger = RecipeOf("Iron Dagger");
    REQUIRE(dagger >= 0, "Iron Dagger recipe exists");

    const int n_w = Rows(f);
    for (int i = 0; i < n_w; i++) {
        if ((int)SortScratch()[i].ref == dagger) f->cursor = (signed char)i;
    }

    const int ore0 = InvHeld(ITM_IRON_ORE);
    const int strip0 = InvHeld(ITM_LEATHER_STRIP);
    const int dag0 = InvHeld(ITM_DAGGER);

    /* 1.6: the accept key names the recipe and spends nothing. The ore leaves
       the pack when the scene starts the heat, not when the row is chosen. */
    const ForgeCommand cmd = ForgeInput(f, 0, 0, true);
    CHECK(ForgeCmdIsBegin(cmd), "an affordable row asks for a heat");
    CHECK(ForgeCmdRecipe(cmd) == dagger, "and names the recipe under the cursor");
    CHECK(InvHeld(ITM_IRON_ORE) == ore0, "choosing a row spends nothing");

    CHECK(InvSpendMaterials(dagger), "the heat takes the ingredients");
    CHECK(InvGrantItem(RECIPES[dagger].out), "and pays out on success");
    printf("forge dagger: ore %d -> %d, strip %d -> %d, daggers %d -> %d\n",
           ore0, InvHeld(ITM_IRON_ORE), strip0, InvHeld(ITM_LEATHER_STRIP),
           dag0, InvHeld(ITM_DAGGER));
    CHECK(InvHeld(ITM_IRON_ORE) == ore0 - 2, "ore debited exactly");
    CHECK(InvHeld(ITM_LEATHER_STRIP) == strip0 - 1, "strip debited exactly");
    CHECK(InvHeld(ITM_DAGGER) == dag0 + 1, "exactly one blade produced");

    /* --- all-or-nothing: a short third ingredient takes nothing --------- */
    const int staff = RecipeOf("Ash Staff");
    REQUIRE(staff >= 0, "Ash Staff recipe exists");
    g_inv.held[ITM_LEATHER_STRIP] = 0;          /* third slot only */
    const int bone0 = InvHeld(ITM_BEAST_BONE);
    const int silver0 = InvHeld(ITM_SILVER_INGOT);
    CHECK(!InvSpendMaterials(staff), "forge refused when one ingredient is short");
    CHECK(InvHeld(ITM_BEAST_BONE) == bone0, "bone untouched by a refusal");
    CHECK(InvHeld(ITM_SILVER_INGOT) == silver0, "silver untouched by a refusal");
    g_inv.held[ITM_LEATHER_STRIP] = 26;

    /* --- the row index is not the recipe index once sorted -------------- */
    /* The shop shipped this exact bug in 1.2: reading the table by row number
       forges whatever landed in that slot after the sort. */
    ForgeSort(f, SORT_ALPHA);
    const int n_sorted = Rows(f);
    REQUIRE(n_sorted >= 2, "weapon shelf has rows to sort");
    bool differs = false;
    for (int i = 0; i < n_sorted; i++) {
        printf("  row %d -> recipe %d (%s)\n", i, (int)SortScratch()[i].ref,
               ITEMS[RECIPES[SortScratch()[i].ref].out].name);
        if ((int)SortScratch()[i].ref != i) differs = true;
    }
    CHECK(differs, "alphabetical order really does move a row off its index");

    f->cursor = 0;
    const int want = ForgeRowRecipe(f, 0);
    CHECK(ForgeCmdRecipe(ForgeInput(f, 0, 0, true)) == want,
          "the row the cursor is on is the one that gets forged");

    /* --- BLUEPRINTS is read-only, and shows both shelves ---------------- */
    ForgeOpen(f, FORGE_MODE_BOOK);
    CHECK(Rows(f) == RECIPE_COUNT, "blueprints lists every known recipe");

    f->tab = FORGE_ARMOR;              /* must be ignored in book mode */
    ForgeInput(f, 1, 0, false);
    CHECK(Rows(f) == RECIPE_COUNT, "tabs do not filter the blueprint list");

    f->cursor = 0;
    CHECK(ForgeInput(f, 0, 0, true) == FORGE_CMD_NONE, "blueprints never craft");

    /* --- an unlearned recipe is absent from both lists ------------------ */
    g_inv.known &= ~(1u << dagger);
    ForgeOpen(f, FORGE_MODE_BOOK);
    CHECK(Rows(f) == RECIPE_COUNT - 1, "an unlearned recipe leaves the book");
    ForgeOpen(f, FORGE_MODE_CRAFT);
    f->tab = FORGE_WEAPON;
    CHECK(Rows(f) == 2, "an unlearned recipe leaves the shelf");
    CHECK(!InvCanForge(dagger), "an unlearned recipe cannot be forged");
    InvLearn(dagger);
    CHECK(Rows(f) == 3, "learning it puts it back");

    /* --- the ESC ladder ------------------------------------------------- */
    SceneInit(&sc, SCENE_SMITHY);
    f = &sc.forge;
    sc.phase = PHASE_DONE;
    UiDialogHide(&sc.dialog);

    /* 1.7: the anvil is a row in the main menu, not the accept key. */
    SceneAdvance(&sc);
    CHECK(!UiPromptIsOpen(&sc.prompt), "accept at an idle screen does nothing");
    REQUIRE(DriveFeature(&sc), "the smithy offers a FORGE row");
    REQUIRE(UiPromptIsOpen(&sc.prompt), "which raises the anvil prompt");
    CHECK(sc.prompt.cursor == 0, "FORGE is the default door");

    SceneAdvance(&sc);
    CHECK(ForgeIsOpen(&sc.forge), "FORGE opens the craft list");
    CHECK(sc.forge.mode == FORGE_MODE_CRAFT, "opened in craft mode");
    CHECK(!UiPromptIsOpen(&sc.prompt), "the prompt closed behind it");

    SceneBack(&sc);
    CHECK(!ForgeIsOpen(&sc.forge), "ESC closes the list");
    CHECK(UiPromptIsOpen(&sc.prompt), "ESC pops exactly one level, to the prompt");

    UiPromptMove(&sc.prompt, 1, 0);
    SceneAdvance(&sc);
    CHECK(ForgeIsOpen(&sc.forge) && sc.forge.mode == FORGE_MODE_BOOK,
          "the second door opens the blueprints");

    SceneBack(&sc);
    SceneBack(&sc);
    CHECK(!ForgeIsOpen(&sc.forge) && !UiPromptIsOpen(&sc.prompt),
          "a second ESC leaves the anvil entirely");

    /* M must not reach past an open list, and the shop room has no anvil. */
    REQUIRE(DriveFeature(&sc), "FORGE row again");
    SceneAdvance(&sc);
    REQUIRE(ForgeIsOpen(&sc.forge), "list open again");
    SceneToggleMenu(&sc);
    CHECK(!UiMenuIsOpen(&sc.menu), "M is refused while the list is up");
    SceneBack(&sc);
    SceneBack(&sc);

    CHECK(SCENES[SCENE_SMITHY].feature == ROOM_FEATURE_FORGE,
          "the smithy's feature is the anvil");
    CHECK(SCENES[SCENE_SHOP].feature == ROOM_FEATURE_TRADE,
          "the shop's is the counter");

    /* --- the save carries the mask and the new held counts -------------- */
    StubClearSave();
    g_inv.known &= ~(1u << staff);
    g_inv.held[ITM_DAGGER] = 5;
    sc.prompt_kind = PROMPT_SYSTEM;
    UiPromptOpen(&sc.prompt, "PAUSED", SYSTEM_OPTIONS, 3, SYS_SAVE);
    SceneAdvance(&sc);

    g_inv.known = 0xFFFFFFFFu;
    g_inv.held[ITM_DAGGER] = 0;
    REQUIRE(SceneLoadSave(&sc), "save round-trips");
    CHECK(!InvKnows(staff), "the unlearned recipe stayed unlearned");
    CHECK(InvHeld(ITM_DAGGER) == 5, "a forged blade survives the save");
    printf("\nsave blob %d bytes\n", (int)sizeof(SaveBlob));

    REPORT();
}
