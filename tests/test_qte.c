/* test_qte.c - the heat: sequence generation, grading, the spend/grant split
   across the minigame, and the one place ESC is refused. */
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

#define TICK (1.0f / 60.0f)

static int RecipeOf(const char *name)
{
    for (int i = 0; i < RECIPE_COUNT; i++) {
        if (ITEMS[RECIPES[i].out].name == name) return i;
    }
    return -1;
}

/* Press the key the sequence is asking for, `wrong` times deliberately not. */
static void PlaySequence(UiQte *q, int wrong)
{
    while (!q->done) {
        const QteKey want = (QteKey)q->step[q->at];
        if (wrong > 0) {
            /* Any other key. QK_KEY_COUNT is 5, so +1 modulo is always
               a different one. */
            QteKeyPress(q, (QteKey)((want + 1) % QK_KEY_COUNT));
            wrong--;
        } else {
            QteKeyPress(q, want);
        }
        QteUpdate(q, TICK);
    }
}

/* Let the verdict sit long enough for the scene to collect it. */
static void SettleVerdict(Scene *s)
{
    for (int i = 0; i < 200 && QteIsOpen(&s->qte); i++) SceneUpdate(s, TICK);
}

int main(void)
{
    Scene sc;
    SceneInit(&sc, SCENE_SMITHY);
    QteSeed(0xC0FFEEu);

    /* --- sequences ------------------------------------------------------ */
    printf("recipe            out tier    steps  sequence\n");
    for (int i = 0; i < RECIPE_COUNT; i++) {
        UiQte q;
        QteBegin(&q, i);
        printf("%-17s %-9s %5d  ", ITEMS[RECIPES[i].out].name,
               RarityName((Rarity)ITEMS[RECIPES[i].out].rarity), q.steps);
        for (int k = 0; k < q.steps; k++) printf("%d", q.step[k]);
        printf("\n");

        CHECK(q.steps >= 2 && q.steps <= QTE_MAX_STEPS, "step count in range");
        CHECK(q.steps == Q_BASE_STEPS + (int)ITEMS[RECIPES[i].out].rarity,
              "length follows the tier of what is being made");
        for (int k = 0; k < q.steps; k++) {
            CHECK(q.step[k] < QK_KEY_COUNT, "every step is a real key");
            if (k) CHECK(q.step[k] != q.step[k - 1],
                         "no step repeats the one before it");
        }
    }

    /* Same seed, same sequence - the tests below depend on it. */
    {
        UiQte a, b;
        QteSeed(99u); QteBegin(&a, 0);
        QteSeed(99u); QteBegin(&b, 0);
        bool same = (a.steps == b.steps);
        for (int i = 0; same && i < a.steps; i++) same = (a.step[i] == b.step[i]);
        CHECK(same, "the generator is deterministic under a seed");
    }

    /* --- grading -------------------------------------------------------- */
    const int sword = RecipeOf("Iron Shortsword");
    REQUIRE(sword >= 0, "Iron Shortsword recipe exists");

    {
        UiQte q;
        QteSeed(1u); QteBegin(&q, sword);
        const int tol = Q_TOLERANCE(q.steps);
        printf("\nshortsword: %d steps, %d misses tolerated\n", q.steps, tol);

        PlaySequence(&q, 0);
        CHECK(QteResult(&q) == QTE_FINE, "a clean run is fine work");

        QteSeed(1u); QteBegin(&q, sword);
        PlaySequence(&q, tol);
        CHECK(QteResult(&q) == QTE_PLAIN, "misses within tolerance still pay");

        QteSeed(1u); QteBegin(&q, sword);
        PlaySequence(&q, tol + 1);
        CHECK(QteResult(&q) == QTE_RUINED, "one miss past tolerance ruins it");
    }

    /* --- an unplayed heat still ends ------------------------------------ */
    {
        UiQte q;
        QteSeed(7u); QteBegin(&q, sword);
        int ticks = 0;
        while (!q.done && ticks < 60 * 30) { QteUpdate(&q, TICK); ticks++; }
        CHECK(q.done, "a player who presses nothing still reaches a verdict");
        CHECK(QteResult(&q) == QTE_RUINED, "and it is a ruined one");
        CHECK(q.misses == q.steps, "every window expired");
        printf("unplayed heat resolved in %.2fs\n", (float)ticks * TICK);
    }

    /* --- the fine output ladder ----------------------------------------- */
    CHECK(QteOutput(sword, QTE_FINE) == ITM_LONGSWORD,
          "a flawless sword is the better sword");
    CHECK(QteOutput(sword, QTE_PLAIN) == ITM_SHORTSWORD, "a plain one is not");
    CHECK(QteOutput(sword, QTE_RUINED) == -1, "a ruined heat makes nothing");

    const int dagger = RecipeOf("Iron Dagger");
    REQUIRE(dagger >= 0, "Iron Dagger recipe exists");
    CHECK(RECIPES[dagger].fine == RECIPE_NONE, "the dagger has no finer self");
    CHECK(QteOutput(dagger, QTE_FINE) == QteOutput(dagger, QTE_PLAIN),
          "a recipe with no ladder falls back to its normal output");

    /* --- the whole cycle through the scene ------------------------------ */
    sc.phase = PHASE_DONE;
    UiDialogHide(&sc.dialog);

    REQUIRE(DriveFeature(&sc), "the smithy offers a FORGE row");
    SceneAdvance(&sc);                       /* THE ANVIL -> FORGE */
    REQUIRE(ForgeIsOpen(&sc.forge), "the craft list opened");
    sc.forge.tab = FORGE_WEAPON;
    SortReset(&sc.forge.sort);

    const int n = ForgeBuildRows(&sc.forge);
    for (int i = 0; i < n; i++) {
        if ((int)SortScratch()[i].ref == sword) sc.forge.cursor = (signed char)i;
    }

    const int ore0 = InvHeld(ITM_IRON_ORE);
    const int coal0 = InvHeld(ITM_COAL);
    const int long0 = InvHeld(ITM_LONGSWORD);
    const int short0 = InvHeld(ITM_SHORTSWORD);

    QteSeed(1u);
    SceneAdvance(&sc);                       /* start the heat */
    REQUIRE(QteIsOpen(&sc.qte), "the minigame opened");
    CHECK(!ForgeIsOpen(&sc.forge), "the list closed behind it");
    CHECK(InvHeld(ITM_IRON_ORE) == ore0 - 6, "the ore went in at the start");
    CHECK(InvHeld(ITM_COAL) == coal0 - 4, "and the coal with it");

    /* ESC is the one key the minigame refuses. */
    SceneBack(&sc);
    CHECK(QteIsOpen(&sc.qte), "ESC cannot abandon a heat");
    SceneToggleMenu(&sc);
    CHECK(!UiMenuIsOpen(&sc.menu), "M is refused too");

    /* Play it perfectly through the scene's own input entry points, which is
       what proves the dx/dy translation is wired to the right keys. */
    while (!sc.qte.done) {
        switch ((QteKey)sc.qte.step[sc.qte.at]) {
        case QK_LEFT:  SceneMove(&sc, -1, 0); break;
        case QK_RIGHT: SceneMove(&sc,  1, 0); break;
        case QK_UP:    SceneMove(&sc,  0, -1); break;
        case QK_DOWN:  SceneMove(&sc,  0,  1); break;
        default:       SceneAdvance(&sc); break;
        }
        SceneUpdate(&sc, TICK);
    }
    CHECK(QteResult(&sc.qte) == QTE_FINE, "played clean through the scene");

    SettleVerdict(&sc);
    CHECK(!QteIsOpen(&sc.qte), "the verdict cleared itself");
    CHECK(ForgeIsOpen(&sc.forge), "and the list came back");
    CHECK(sc.forge.result != NULL, "with a line saying how it went");
    printf("verdict line: %s\n", sc.forge.result);

    CHECK(InvHeld(ITM_LONGSWORD) == long0 + 1, "a fine heat paid the better blade");
    CHECK(InvHeld(ITM_SHORTSWORD) == short0, "and not the ordinary one");

    /* --- a ruined heat costs and pays nothing --------------------------- */
    {
        const int ore1 = InvHeld(ITM_IRON_ORE);
        const int long1 = InvHeld(ITM_LONGSWORD);
        const int short1 = InvHeld(ITM_SHORTSWORD);

        const int n2 = ForgeBuildRows(&sc.forge);
        for (int i = 0; i < n2; i++) {
            if ((int)SortScratch()[i].ref == sword) sc.forge.cursor = (signed char)i;
        }
        SceneAdvance(&sc);
        REQUIRE(QteIsOpen(&sc.qte), "second heat opened");
        CHECK(InvHeld(ITM_IRON_ORE) == ore1 - 6, "ore spent again");

        for (int i = 0; i < 60 * 30 && !sc.qte.done; i++) SceneUpdate(&sc, TICK);
        CHECK(QteResult(&sc.qte) == QTE_RUINED, "ignored it, ruined it");
        SettleVerdict(&sc);

        CHECK(InvHeld(ITM_IRON_ORE) == ore1 - 6, "the ore stays spent");
        CHECK(InvHeld(ITM_LONGSWORD) == long1, "nothing fine was made");
        CHECK(InvHeld(ITM_SHORTSWORD) == short1, "nothing plain either");
        printf("ruined line: %s\n", sc.forge.result);
    }

    /* --- travel mid-heat cannot strand the minigame ---------------------- */
    {
        const int n3 = ForgeBuildRows(&sc.forge);
        for (int i = 0; i < n3; i++) {
            if ((int)SortScratch()[i].ref == dagger) sc.forge.cursor = (signed char)i;
        }
        SceneAdvance(&sc);
        REQUIRE(QteIsOpen(&sc.qte), "third heat opened");
        SceneReset(&sc);
        CHECK(!QteIsOpen(&sc.qte), "reloading the room closes the minigame");
    }

    REPORT();
}
