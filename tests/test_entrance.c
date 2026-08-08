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


int main(void)
{
    Scene s;
    SceneInit(&s, SCENE_SHOP);
    const SceneDef *d = &SCENES[SCENE_SHOP];

    printf("shop actors: %d, settle at %.2fs\n", s.actors, (double)SettleTime(d));
    CHECK(s.actors == 2, "two actors load");
    CHECK(d->actor[0].enter_dir == +1, "BEST from the right");
    CHECK(d->actor[1].enter_dir == -1, "JACK from the left");
    CHECK(d->actor[1].delay > 1.25f && d->actor[1].delay < 1.35f,
          "JACK is on a 1.3s delay");
    CHECK(SettleTime(d) < 2.6f, "the whole entrance costs under 2.6s");
    CHECK(d->actor[1].rest_y > d->actor[0].rest_y, "JACK's head is lower = shorter");

    printf("\n  t     BEST x   JACK x   phase  dialog\n");
    float t = 0.0f;
    bool best_done = false, jack_seen = false;
    float best_settled_at = -1.0f, jack_started_at = -1.0f;
    for (int f = 0; f < 300; f++) {
        SceneUpdate(&s, 1.0f/60.0f);
        t += 1.0f/60.0f;

        if (!best_done && s.actor_x[0] == (float)d->actor[0].rest_x && t > 0.5f) {
            best_done = true; best_settled_at = t;
        }
        if (!jack_seen && s.actor_x[1] > -(float)VSCREEN_W) {
            jack_seen = true; jack_started_at = t;
        }
        if (f % 30 == 0) {
            printf("  %4.2f  %7.0f  %7.0f   %d      %s\n", (double)t,
                   (double)s.actor_x[0], (double)s.actor_x[1], s.phase,
                   s.dialog.phase == DIALOG_HIDDEN ? "-" : s.dialog.line->speaker);
        }
    }
    printf("\nBEST settled at %.2fs, JACK started moving at %.2fs (gap %.2fs)\n",
           (double)best_settled_at, (double)jack_started_at,
           (double)(jack_started_at - best_settled_at));
    /* 1.3: the two walks deliberately overlap - JACK sets off while BEST is
       still crossing the last 0.3 s of floor, which is what two people
       entering a room looks like. What must hold is that JACK still lands
       last, so the script waits for him. */
    CHECK(jack_started_at < best_settled_at, "the walks overlap");
    CHECK(SettleTime(d) > best_settled_at, "JACK still lands last");

    CHECK(s.actor_x[0] == (float)d->actor[0].rest_x, "BEST at his mark");
    CHECK(s.actor_x[1] == (float)d->actor[1].rest_x, "JACK at his mark");
    CHECK(s.phase != PHASE_ENTER, "room settled");
    REQUIRE(s.dialog.line != NULL, "a line is on screen once the room settles");
    CHECK(s.dialog.line->set == PORTRAIT_MERCHANT,
          "JACK speaks first, with his own portrait");

    /* Skipping must land the whole room, not just whoever was walking. */
    Scene k; SceneInit(&k, SCENE_SHOP);
    SceneUpdate(&k, 1.0f/60.0f);
    SceneAdvance(&k);
    CHECK(k.actor_x[0] == (float)d->actor[0].rest_x &&
          k.actor_x[1] == (float)d->actor[1].rest_x, "skip lands both actors");
    CHECK(k.phase == PHASE_SETTLED, "skip settles the room");
    printf("skip: BEST %.0f JACK %.0f phase %d\n",
           (double)k.actor_x[0], (double)k.actor_x[1], k.phase);

    /* Smithy still behaves. */
    Scene m; SceneInit(&m, SCENE_SMITHY);
    CHECK(m.actors == 1, "smithy has one actor");
    for (int f = 0; f < 200; f++) SceneUpdate(&m, 1.0f/60.0f);
    CHECK(m.actor_x[0] == 239.0f, "BEST reaches his smithy mark");
    CHECK(m.dialog.line != NULL && m.dialog.line->set == PORTRAIT_HERO,
          "smithy uses the hero strip");

    SceneUnload(&s); SceneUnload(&k); SceneUnload(&m);
    REPORT();
}
