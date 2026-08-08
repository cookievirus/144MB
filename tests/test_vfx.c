/* test_vfx.c - the hearth. A look cannot be asserted, but the things that
   make a look go wrong can be: particles escaping their room, a flicker that
   sits still or clips, a generator that reaches into the forge minigame's,
   and an effect that keeps running in a room that does not have one. */
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

int main(void)
{
    /* --- the data matches the art -------------------------------------- */
    /* Measured from resource/BG-01-SMITTY-A03.png: the lit hearth mouth is
       x 216..247, y 110..131. If someone re-crops the backdrop and forgets
       this table, the effect drifts off the fire and these fail. */
    const LightDef *d = &SCENES[SCENE_SMITHY].light[0];
    printf("hearth at (%d,%d) r %dx%d, bed %d wide %d down, rise %d, %d embers\n",
           d->x, d->y, d->rx, d->ry, d->bed_w, d->bed_dy, d->rise, d->embers);

    CHECK(d->embers > 0, "the smithy has a fire");
    /* The glow is centred on the painted fire; its reach is allowed to spill
       well past it, because that is what light does. What must not drift is
       the centre. */
    CHECK(d->x >= 216 && d->x <= 247, "the glow centres on the painted hearth");
    CHECK(d->y >= 110 && d->y <= 131, "and on its middle vertically");
    CHECK((int)d->x - (int)d->rx > -32 && (int)d->x + (int)d->rx < VSCREEN_W + 32,
          "and its reach is not absurd");
    CHECK(d->embers <= VFX_EMBERS, "the room cannot ask for more than the array");
    CHECK(SCENES[SCENE_SHOP].lights == 3, "the shop has three lamps");

    /* --- it lights with the room, and goes out with it ------------------ */
    Scene sc;
    SceneInit(&sc, SCENE_SMITHY);
    CHECK(VfxRoomIsLive(&sc.ambience), "the hearth is lit on entry");

    /* Nothing is born stacked on the coals: a cold start would put all
       fourteen on the same pixel row and the first second would be a pulse. */
    int distinct_y = 0;
    for (int i = 0; i < d->embers; i++) {
        bool seen = false;
        for (int j = 0; j < i; j++) {
            if ((int)sc.ambience.ember[j].y == (int)sc.ambience.ember[i].y) seen = true;
        }
        if (!seen) distinct_y++;
    }
    printf("embers occupy %d distinct rows at t=0\n", distinct_y);
    CHECK(distinct_y >= 4, "the column is already populated when the room opens");

    /* --- particles stay in their room ----------------------------------- */
    const int top = (int)d->y + (int)d->bed_dy - (int)d->rise;
    const int left = (int)d->x - (int)d->bed_w - 16;
    const int right = (int)d->x + (int)d->bed_w + 16;
    int escapes = 0, respawns = 0, lo = 9999, hi = -9999;

    for (int frame = 0; frame < 60 * 30; frame++) {
        SceneUpdate(&sc, TICK);
        for (int i = 0; i < d->embers; i++) {
            const Ember *e = &sc.ambience.ember[i];
            const int px = (int)e->x, py = (int)e->y;
            if (py < lo) lo = py;
            if (py > hi) hi = py;
            /* An ember above the rise height is not drawn, but it must not
               have wandered off sideways or fallen through the floor. */
            if (px < left || px > right) escapes++;
            if (py > (int)d->y + (int)d->bed_dy + 4) escapes++;
            if (e->life > e->span + 0.001f) escapes++;
            if (e->life <= 0.0f) respawns++;
        }
    }
    printf("30s: y range %d..%d (draw floor %d), escapes %d, dead-on-read %d\n",
           lo, hi, top, escapes, respawns);
    CHECK(escapes == 0, "no ember leaves its hearth in half a minute");
    CHECK(respawns == 0, "and none is ever left dead between frames");
    CHECK(lo < (int)d->y, "they do rise above the coals");
    /* An ember may climb past `rise` - a slow, long-lived one does, by nine
       pixels here - which is exactly why `rise` fades them out rather than
       clipping them. A clip put a visible blink at y=100 in the first cut of
       this effect, and this is the trace that found it. */
    CHECK(lo < top, "some outlive the fade envelope, so it must be a fade");

    /* --- the flicker moves, and stays in range -------------------------- */
    float gmin = 2.0f, gmax = -1.0f, prev = VfxLightGlow(&sc.ambience, 0);
    int changes = 0;
    for (int frame = 0; frame < 60 * 10; frame++) {
        SceneUpdate(&sc, TICK);
        const float g = VfxLightGlow(&sc.ambience, 0);
        if (g < gmin) gmin = g;
        if (g > gmax) gmax = g;
        if (g != prev) changes++;
        prev = g;
    }
    printf("glow over 10s: %.3f .. %.3f, changed on %d of 600 frames\n",
           (double)gmin, (double)gmax, changes);
    CHECK(gmin >= V_TUNE[LIGHT_HEARTH].floor - 0.001f,
          "the forge never goes fully dark");
    CHECK(gmax <= 1.0f, "and never overdrives");
    CHECK(gmax - gmin > 0.15f, "the flicker actually flickers");
    CHECK(changes > 500, "and moves nearly every frame rather than stepping");

    /* --- it does not reach into the minigame's generator ----------------- */
    /* Sharing a seed would make a forge sequence depend on how long the
       player stared at the fire, and test_qte asserts exact sequences. */
    UiQte a, b;
    QteSeed(4242u);
    QteBegin(&a, 0);
    QteSeed(4242u);
    for (int frame = 0; frame < 137; frame++) VfxRoomUpdate(&sc.ambience, TICK);
    VfxRoomDraw(&sc.ambience);
    QteBegin(&b, 0);
    bool same = (a.steps == b.steps);
    for (int i = 0; same && i < a.steps; i++) same = (a.step[i] == b.step[i]);
    CHECK(same, "the hearth does not perturb the forge's sequence");

    /* --- a room without a fire runs nothing ------------------------------ */
    Scene shop;
    SceneInit(&shop, SCENE_SHOP);
    CHECK(VfxRoomIsLive(&shop.ambience), "the shop is lit too");
    CHECK(shop.ambience.sparker == -1, "but nothing in it throws sparks");

    /* Warm-weighted centroids off resource/BG-01-ITEM_SHOP-A01.png: a wall
       lantern at (12.8,62.6) and hanging lamps at (224.0,29.0) and
       (252.2,29.4). The first threshold pass found the doorway and the
       skylight instead - both far brighter than a wick - which is why these
       are pinned rather than trusted.

       Pinned to within a pixel rather than exactly. The measurement is a
       centroid of a resampled image and lands on a fraction; asserting an
       integer it happens to round to would fail on a different resampler
       while telling nobody anything useful. A pixel is the tolerance that
       distinguishes "the glow is on the lamp" from "the glow is on the
       shelf", which is the thing worth failing over. */
    static const int LAMP[3][2] = { { 13, 63 }, { 224, 29 }, { 252, 29 } };
    #define NEAR(a, b) (((a) - (b) <= 1) && ((b) - (a) <= 1))
    for (int i = 0; i < 3; i++) {
        const LightDef *l = &SCENES[SCENE_SHOP].light[i];
        printf("lamp %d at (%d,%d) r %dx%d kind %d embers %d\n",
               i, l->x, l->y, l->rx, l->ry, l->kind, l->embers);
        CHECK(l->kind == LIGHT_LAMP, "a shop lamp is a lamp, not a forge");
        CHECK(l->embers == 0, "and a lamp in a potion shop throws no embers");
        CHECK(NEAR((int)l->x, LAMP[i][0]) && NEAR((int)l->y, LAMP[i][1]),
              "and sits within a pixel of where the art puts it");
        CHECK(l->rx == l->ry, "a lamp throws a circle, not a lens");
        CHECK(l->rx >= 8, "and one big enough to read as light, not a dot");
        CHECK((int)l->x + (int)l->rx <= VSCREEN_W + 8,
              "its halo does not run off the right edge");
    }

    /* A lamp is steadier than a forge, which is the whole reason kinds exist:
       one tuning for both would make either the wick roar or the forge sulk. */
    float lmin = 2.0f, lmax = -1.0f;
    for (int frame = 0; frame < 60 * 10; frame++) {
        SceneUpdate(&shop, TICK);
        const float g = VfxLightGlow(&shop.ambience, 0);
        if (g < lmin) lmin = g;
        if (g > lmax) lmax = g;
    }
    printf("lamp glow over 10s: %.3f .. %.3f (hearth swung %.3f)\n",
           (double)lmin, (double)lmax, (double)(gmax - gmin));
    CHECK(lmin >= V_TUNE[LIGHT_LAMP].floor - 0.001f, "a lamp has a floor too");
    CHECK(lmax - lmin > 0.05f, "it does move");
    CHECK(lmax - lmin < gmax - gmin, "but less than the forge does");

    /* Three lamps on one clock would breathe in unison and give the whole
       effect away as a single timer. */
    int apart = 0;
    for (int frame = 0; frame < 300; frame++) {
        SceneUpdate(&shop, TICK);
        const float a0 = VfxLightGlow(&shop.ambience, 0);
        const float a1 = VfxLightGlow(&shop.ambience, 1);
        const float a2 = VfxLightGlow(&shop.ambience, 2);
        if (a0 != a1 && a1 != a2 && a0 != a2) apart++;
    }
    printf("all three lamps differed on %d of 300 frames\n", apart);
    CHECK(apart > 280, "each lamp keeps its own phase");

    CHECK(VfxLightGlow(&shop.ambience, 3) == 0.0f, "an index past the end is dark");
    CHECK(VfxLightGlow(&shop.ambience, -1) == 0.0f, "and so is a negative one");
    SceneUnload(&shop);

    /* A NULL definition is the same as no fire, so a room added without one
       cannot fault. */
    VfxRoom bare;
    VfxRoomStart(&bare, NULL, 0);
    CHECK(!VfxRoomIsLive(&bare), "a NULL light table is simply an unlit room");
    CHECK(bare.sparker == -1, "with nothing to spark");
    VfxRoomUpdate(&bare, TICK);
    VfxRoomDraw(&bare);

    /* --- travel puts it out and lights the right one --------------------- */
    BeginTravel(&sc, SCENE_SHOP);
    for (int frame = 0; frame < 120; frame++) SceneUpdate(&sc, TICK);
    CHECK(sc.id == SCENE_SHOP, "travelled");
    CHECK(sc.ambience.sparker == -1, "the smithy's embers did not come along");
    CHECK(sc.ambience.count == 3, "the shop's own lamps lit instead");
    BeginTravel(&sc, SCENE_SMITHY);
    for (int frame = 0; frame < 120; frame++) SceneUpdate(&sc, TICK);
    CHECK(VfxRoomIsLive(&sc.ambience), "and the hearth is lit again on the way back");
    CHECK(sc.ambience.sparker == 0, "sparking once more");

    /* --- the widened date badge ------------------------------------------ */
    printf("date badge %dx%d at (%d,%d)\n",
           HUD_DAY_W, HUD_DAY_H, HUD_DAY_X, HUD_DAY_Y);
    CHECK(HUD_DAY_Y + HUD_DAY_H <= PAGE_Y + 7,
          "the badge still clears every page's title line");
    CHECK(HUD_DAY_X + HUD_DAY_W < VSCREEN_W, "and stays on screen");
    CHECK(HUD_DAY_W >= 78, "and is no longer cramped");
    CHECK(HUD_DAY_PAD * 2 + FONT_CELL_W * (3 + HUD_DAY_DIGITS) + HUD_DAY_GAP
          <= HUD_DAY_W, "with room for DAY, a gap, and five digits");

    /* --- the build stamp -------------------------------------------------- */
    printf("version stamp \"%s\", %d px, right edge at %d\n",
           GAME_VERSION, UiTextWidth(GAME_VERSION), HUD_VER_X);
    CHECK(HUD_VER_X - UiTextWidth(GAME_VERSION) > HUD_HINT_X + UiTextWidth(HUD_HINT),
          "the version stamp clears the menu hint at the other end of the row");
    CHECK(HUD_VER_X <= VSCREEN_W, "and stays on screen");
    CHECK(HUD_VER_Y + 7 <= VSCREEN_H, "and inside the frame");
    CHECK(sizeof(GAME_VERSION) > 1, "there is a version to draw");

    SceneUnload(&sc);
    REPORT();
}
