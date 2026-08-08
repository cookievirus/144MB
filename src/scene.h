/* scene.h - one room, chosen from a table.

   1.1 had scene_smithy.{h,c}: a room welded to a file. Adding the shop that
   way would have meant a second copy of the same entrance state machine,
   dialogue pump, ESC ladder and save path, and a third room would have made
   it three copies. Both rooms are the same shape - a backdrop, at most one
   standing actor, an intro script and a solo script - so the shape is a
   struct in .rodata and the code exists once.

   A room now costs one SceneDef row plus its scripts. The cost of the move is
   that a room with genuinely unique behaviour cannot express it here; when
   the forge minigame lands in Week 2 it goes behind a per-scene hook rather
   than into this file. */
#ifndef SCENE_H
#define SCENE_H

#include "forge.h"
#include "gfx.h"
#include "qte.h"
#include "version.h"
#include "vfx.h"
#include "shop.h"
#include "sort.h"
#include "ui_dialog.h"
#include "ui_menu.h"
#include "ui_prompt.h"

typedef enum SceneId {
    SCENE_SMITHY = 0,
    SCENE_SHOP,
    SCENE_COUNT
} SceneId;

/* Must agree with DESTINATIONS[].scene in game_data.h, which stores SceneId
   as a byte and uses SCENE_NONE for a room that does not exist yet. */
_Static_assert(SCENE_COUNT < SCENE_NONE, "SceneId has outgrown its byte");

typedef enum ScenePhase {
    PHASE_ENTER = 0, /* somebody is still walking on   */
    PHASE_SETTLED,   /* everyone at rest, script running */
    PHASE_DONE       /* script finished                  */
} ScenePhase;

/* PHASE_HOLD is gone. 1.2's first cut held the empty room for 0.45 s and then
   started the one actor, which stopped working the moment a room needed two
   people arriving at different times. A per-actor delay says the same thing
   and says the rest of it too: the hold is just BEST's delay. */

#define SCENE_MAX_ACTORS 2

typedef struct SceneActor {
    const EmbeddedImage *img;
    short rest_x, rest_y;       /* virtual pixels, top-left at rest */
    signed char enter_dir;      /* +1 from the right, -1 left, 0 already there */
    float delay;                /* seconds after the room opens */
} SceneActor;

typedef struct SceneDef {
    const EmbeddedImage *bg;
    SceneActor actor[SCENE_MAX_ACTORS];
    unsigned char actors;           /* 0 for an empty room */
    /* 1.7. One RoomFeature byte where 1.4 had has_shop and 1.5 added
       has_forge. Two flags claimed a room could have a counter and an anvil
       at once; no room does, and the main menu could not have drawn it if one
       did - the root grid has a single slot for what the room is for. */
    unsigned char feature;
    /* 1.9.1. Everything burning in this room, by value. `lights == 0` is a
       room with nothing lit, which no room is today but the Guild might be
       before its brazier is drawn. Twelve bytes a slot; four slots. */
    LightDef light[SCENE_MAX_LIGHTS];
    unsigned char lights;
    const DialogLine *intro;
    const DialogLine *solo;         /* played by the TALK command */
    unsigned char intro_len, solo_len;
} SceneDef;

extern const SceneDef SCENES[SCENE_COUNT];

typedef enum FadeKind {
    FADE_TRAVEL = 0,
    FADE_DAY
} FadeKind;

typedef struct Scene {
    unsigned char id;
    Texture2D bg;
    Texture2D actor[SCENE_MAX_ACTORS];
    unsigned char actors;           /* how many actually loaded */

    ScenePhase phase;
    float clock;                    /* seconds since the room opened */
    float actor_x[SCENE_MAX_ACTORS];

    /* Cross-fade to black and back. A hard cut between two full-screen
       backdrops reads as a dropped frame rather than as a door. */
    float fade;                 /* 0 = clear, 1 = black */
    signed char fade_dir;       /* +1 going dark, -1 coming back, 0 idle */
    unsigned char fade_target;  /* SceneId to load at full black */
    /* What the black is hiding. Travel swaps the room; a day boundary keeps
       it and turns the world over instead. Reusing the fade rather than
       writing a second one keeps the two transitions the same length and the
       same curve, which is most of why they read as the same kind of event. */
    unsigned char fade_kind;    /* FadeKind */

    /* Day one is the first day, not the zeroth. Sixteen bits because a run
       that reaches 65535 days has other problems. */
    unsigned short day;

    UiDialog dialog;
    UiMenu menu;
    UiPrompt prompt;
    UiShop shop;
    UiForge forge;
    UiQte qte;
    VfxRoom ambience;

    /* One UiPrompt serves the pause box, the leave-the-counter box and the
       smithy's FORGE / BLUEPRINTS question, so the accept handler has to know
       which one is on screen. */
    unsigned char prompt_kind;

    /* What happens when the current script runs out. This is what carries
       "and then" across a dialogue, so the dialogue pump never has to know
       what a shop is. */
    unsigned char after_script;   /* AfterScript */

    const DialogLine *script;
    int script_len;
    int script_at;

    bool quit_requested;
} Scene;

void SceneInit(Scene *s, SceneId start);

/* True when a save exists and is one this build can read. Checked before the
   title screen offers LOAD, so a dead menu row is never shown. */
bool SceneSaveExists(void);

/* Restores gold, held counts and the room. Returns false and leaves `s`
   untouched if the file is missing, truncated or from another version. */
bool SceneLoadSave(Scene *s);
void SceneReset(Scene *s);                 /* replay the current room */
void SceneUpdate(Scene *s, float dt);

void SceneAdvance(Scene *s);               /* accept key */
void SceneToggleMenu(Scene *s);
void SceneBack(Scene *s);                  /* ESC: one level, always */
void SceneMove(Scene *s, int dx, int dy);

/* R / T / A. Routed to whichever list is on top; ignored when none is. */
void SceneSort(Scene *s, SortMode mode);

bool SceneWantsQuit(const Scene *s);
void SceneDraw(const Scene *s);
void SceneUnload(Scene *s);

#endif /* SCENE_H */
