#include "scene.h"

#include <math.h>

#include "../assets/bg_smithy.h"
#include "../assets/bg_shop.h"
#include "../assets/hero_idle.h"
#include "../assets/merchant_idle.h"

/* ---- scripts ----------------------------------------------------------- */

/* Lines are hand-wrapped: the balloon holds 39 columns and three rows, and a
   wrapper would cost code for no benefit while the script is authored. */
static const DialogLine SMITHY_INTRO[] = {
{ "BEST", "The forge is lit. Iron does not care\nwho is watching, only who is willing\nto swing.", MOOD_STERN, PORTRAIT_HERO },
{ "BEST", "Every blade I sink coin into comes\nback with a story. Some of them come\nback with the party still breathing.", MOOD_SMIRK, PORTRAIT_HERO },
{ "BEST", "Coal is low and the ledger is worse.\nMarket Row first, then. Jack always has\ncharcoal, and always has an opinion.", MOOD_GLUM, PORTRAIT_HERO },
};

static const DialogLine SMITHY_SOLO[] = {
{ "BEST", "Talking to an empty forge again.\nThe coal never argues, at least.", MOOD_SIGH, PORTRAIT_HERO },
{ "BEST", "Thirty-four lumps of ore, eighteen\nsacks of charcoal, and one cracked\nanvil horn. That is the whole empire.", MOOD_GLUM, PORTRAIT_HERO },
{ "BEST", "Right. Enough. Something in here has\nto get hit today.", MOOD_SMIRK, PORTRAIT_HERO },
};

/* The shop script only starts once both men are standing still, so JACK's
   first line lands as he arrives rather than being shouted from off-screen. */
static const DialogLine SHOP_INTRO[] = {
{ "JACK", "Coming, coming! Mind the slimes, they\nbite the ankles of anyone who haggles.", MOOD_LAUGH, PORTRAIT_MERCHANT },
{ "BEST", "You keep a back room, Jack.", MOOD_STERN, PORTRAIT_HERO },
{ "JACK", "I keep a back room. Ore, coal, oil,\nbread out here. Everything a smith\nburns through and hates paying for.", MOOD_SMILE, PORTRAIT_MERCHANT },
{ "BEST", "And you buy back at half.", MOOD_SMIRK, PORTRAIT_HERO },
{ "JACK", "I buy back at half. Somebody has to\ncarry the risk, and it will not be the\nman who already spent the coin.", MOOD_SMIRK, PORTRAIT_MERCHANT },
};

/* TALK, once the welcome is over. Deliberately not the welcome again: a
   shopkeeper who greets you identically every time you speak to him stops
   being a person and becomes a button. This one assumes you are here to
   trade, and the counter opens when he finishes. */
/* Said as the counter closes. The question it ends on is answered by the
   prompt that follows, so the line has to end on one. */
static const DialogLine SHOP_AGAIN[] = {
{ "JACK", "Right, that is you sorted. Anything\nelse before you go?", MOOD_SMILE, PORTRAIT_MERCHANT },
};

static const DialogLine SHOP_BYE[] = {
{ "JACK", "Mind the slimes on the way out. They\nremember who haggled.", MOOD_LAUGH, PORTRAIT_MERCHANT },
};

static const DialogLine SHOP_SOLO[] = {
{ "JACK", "Back again. Something you need, or\nsomething you are trying to get rid of?", MOOD_SMIRK, PORTRAIT_MERCHANT },
{ "JACK", "Either way the shelves are that way\nand my prices have not moved since\nbreakfast.", MOOD_SMILE, PORTRAIT_MERCHANT },
};

#define LEN(a) ((unsigned char)(sizeof(a) / sizeof((a)[0])))

/* ---- the rooms ---------------------------------------------------------

   Layout, in virtual-screen pixels. BEST is 184 tall but only his top ~169
   rows are on screen: the rest falls past y=240 and the screen clips it.
   Cropping the legs lets him be drawn larger without breaking the room's
   perspective, because the framing reads as "close to camera" rather than
   "giant standing in the room". JACK is clipped the same way.

   The shop's numbers are not eyeballed and not measured off a pixel diff
   either - REF-SHOP-COMPOSITE-02.png is a re-render rather than the plate
   with sprites pasted on, so a diff picks up as much changed backdrop as
   changed character. Each sprite is instead template-matched against the
   reference: render it at a candidate height, slide it, and keep the
   position and height with the lowest mean absolute error over its own
   opaque pixels. Both surfaces have a sharp single minimum.

     BEST  h=184  (230, 71)   error 11.8, next best 18.6
     JACK  h=168  ( 13, 88)   error  9.5, next best 11.5

   So JACK is 16 px shorter and his head sits 17 px lower. That is a milder
   difference than the first reference asked for (h=152, a 30 px head gap);
   the second reference is the one that ships.

   JACK arrives from the left, out of the door marked PRIVATE. A shopkeeper
   already standing at his counter when the customer walks in has no reason to
   say "coming".

   The timeline, all from the moment the room opens:

     0.45  BEST starts walking in from the right
     1.30  JACK starts walking in from the left
     1.60  BEST at his mark
     2.45  JACK at his mark, both settled, SHOP_INTRO starts

   1.2 had JACK on a 2.95 s delay, which read as a wait rather than as an
   entrance: BEST stood alone for a second and a half with nothing happening.
   At 1.30 s the two walks overlap by 0.3 s, which is what two people coming
   into a room actually looks like, and the whole entrance now costs 2.45 s
   instead of 4.10 s.

   Delays are measured from the room opening rather than from the previous
   actor, because one clock is what lets the whole entrance be four numbers in
   a table instead of a chain of callbacks - and it makes the skip on the
   accept key a single assignment. */
const SceneDef SCENES[SCENE_COUNT] = {
    [SCENE_SMITHY] = {
        .bg = &bg_smithy,
        .actors = 1,
        .actor = {
            { &hero_idle, 239, 73, +1, 0.45f },
        },
        .has_shop = 0,
        .intro = SMITHY_INTRO, .intro_len = LEN(SMITHY_INTRO),
        .solo  = SMITHY_SOLO,  .solo_len  = LEN(SMITHY_SOLO),
    },
    [SCENE_SHOP] = {
        .bg = &bg_shop,
        .actors = 2,
        .actor = {
            { &hero_idle,     230,  71, +1, 0.45f },
            { &merchant_idle,  13,  88, -1, 1.30f },
        },
        .has_shop = 1,
        .intro = SHOP_INTRO, .intro_len = LEN(SHOP_INTRO),
        .solo  = SHOP_SOLO,  .solo_len  = LEN(SHOP_SOLO),
    },
};

/* ---- system prompt ----------------------------------------------------- */

/* CANCEL is first and is the default, so a stray ESC followed by a stray
   confirm is harmless. */
static const char *const SYSTEM_OPTIONS[3] = { "CANCEL", "SAVE", "QUIT" };
#define SYS_CANCEL 0
#define SYS_SAVE   1
#define SYS_QUIT   2

/* Leaving the counter. Same shape, same widget, different question - which is
   why UiPrompt was written generically in 1.1 rather than as a quit box.
   STAY is first and default, so a reflexive ESC-then-confirm keeps trading. */
/* JACK asks this one out loud first; the prompt is only how the answer is
   collected. YES is the default because a player who reflexively confirms is
   far more likely to have more shopping than to have meant to walk out. */
static const char *const AGAIN_OPTIONS[2] = { "YES", "NO" };
#define AGAIN_YES 0
#define AGAIN_NO  1

typedef enum PromptKind {
    PROMPT_SYSTEM = 0,
    PROMPT_ANYTHING_ELSE
} PromptKind;

/* What a finished script hands control to. */
typedef enum AfterScript {
    AFTER_NONE = 0,
    AFTER_OPEN_SHOP,     /* fresh counter                        */
    AFTER_RESUME_SHOP,   /* the same counter, state intact       */
    AFTER_ASK_AGAIN,     /* raise the "anything else?" prompt    */
    AFTER_LEAVE_SHOP     /* walk out of the room                 */
} AfterScript;

static void OpenSystemPrompt(Scene *s)
{
    s->prompt_kind = PROMPT_SYSTEM;
    UiPromptOpen(&s->prompt, "PAUSED", SYSTEM_OPTIONS, 3, SYS_CANCEL);
}

/* ---- save -------------------------------------------------------------- */

/* Versioned from the first byte so an old save is recognised and rejected
   rather than misread. 1.2 adds gold, the room the player is standing in and
   the held counts, so the version goes to 2 and a v1 blob is refused: the
   fields it lacks have no safe default now that coin can be spent. */
#define SAVE_VERSION 2
#define SAVE_PATH "iron.sav"

typedef struct SaveBlob {
    unsigned char magic[4];        /* 'I','R','O','N' */
    unsigned short version;
    unsigned short scene;
    unsigned short phase;
    unsigned short script_at;
    int gold;
    unsigned short held[ITEM_COUNT];
} SaveBlob;

/* A version 1 blob has no gold field, and there is no honest default for it
   now that coin can be spent - zero would rob the player, GOLD_START would
   pay them twice. So an old save is refused rather than guessed at. */
static bool ReadSave(SaveBlob *out)
{
    if (!FileExists(SAVE_PATH)) return false;

    int size = 0;
    unsigned char *raw = LoadFileData(SAVE_PATH, &size);
    if (raw == NULL) return false;

    bool ok = (size == (int)sizeof(SaveBlob));
    if (ok) {
        const SaveBlob *b = (const SaveBlob *)(const void *)raw;
        ok = b->magic[0] == 'I' && b->magic[1] == 'R' &&
             b->magic[2] == 'O' && b->magic[3] == 'N' &&
             b->version == SAVE_VERSION && b->scene < SCENE_COUNT;
        if (ok) *out = *b;
    }
    UnloadFileData(raw);
    return ok;
}

bool SceneSaveExists(void)
{
    SaveBlob blob;
    return ReadSave(&blob);
}

static bool WriteSave(const Scene *s)
{
    SaveBlob blob = {
        { 'I', 'R', 'O', 'N' }, SAVE_VERSION,
        (unsigned short)s->id, (unsigned short)s->phase,
        (unsigned short)s->script_at, InvGold(), { 0 }
    };
    for (int i = 0; i < ITEM_COUNT; i++) {
        blob.held[i] = (unsigned short)InvHeld(i);
    }
    return SaveFileData(SAVE_PATH, &blob, (int)sizeof(blob));
}

/* ---- room loading ------------------------------------------------------ */

static void PlayScript(Scene *s, const DialogLine *lines, int len,
                       AfterScript then)
{
    s->after_script = (unsigned char)then;
    if (lines == NULL || len <= 0) return;
    s->script = lines;
    s->script_len = len;
    s->script_at = 0;
    UiDialogShow(&s->dialog, &lines[0]);
}

/* The welcome leads straight into the counter: the player travelled here to
   trade, and making them open a menu to reach the thing the room exists for
   is a step that only ever gets in the way. */
static void PlayIntro(Scene *s, const SceneDef *def)
{
    PlayScript(s, def->intro, def->intro_len,
               def->has_shop ? AFTER_OPEN_SHOP : AFTER_NONE);
}

#define ENTER_SECONDS 1.15f
#define FADE_SECONDS  0.28f

/* Where an actor stands before its entrance begins. A whole screen width off
   the edge rather than a few pixels, so a wide sprite is fully clear of the
   frame no matter which side it comes from. */
static float StartX(const SceneActor *a)
{
    if (a->enter_dir > 0) return (float)VSCREEN_W + 4.0f;
    if (a->enter_dir < 0) return -(float)VSCREEN_W;
    return (float)a->rest_x;
}

/* When the last actor finishes walking on. The script waits for this, so a
   line is never delivered by somebody still sliding across the floor. */
static float SettleTime(const SceneDef *def)
{
    float last = 0.0f;
    for (int i = 0; i < def->actors; i++) {
        const float t = def->actor[i].delay + ENTER_SECONDS;
        if (t > last) last = t;
    }
    return last;
}

static void UnloadRoom(Scene *s)
{
    for (int i = 0; i < s->actors; i++) UnloadTexture(s->actor[i]);
    s->actors = 0;
    UnloadTexture(s->bg);
}

static void LoadRoom(Scene *s, SceneId id)
{
    const SceneDef *def = &SCENES[id];
    s->id = (unsigned char)id;
    s->bg = GfxLoadTexture(def->bg);

    /* Textures are released on the way out rather than kept resident for
       every room. At 320x240 either choice is cheap today; unloading keeps
       VRAM flat as the room count grows, and a reload is a few milliseconds
       hidden inside the fade that is already playing. */
    s->actors = def->actors;
    for (int i = 0; i < def->actors; i++) {
        s->actor[i] = GfxLoadTexture(def->actor[i].img);
        s->actor_x[i] = StartX(&def->actor[i]);
    }

    s->phase = (SettleTime(def) > 0.0f) ? PHASE_ENTER : PHASE_SETTLED;
    s->clock = 0.0f;

    s->script = NULL;
    s->script_len = 0;
    s->script_at = 0;

    UiMenuClose(&s->menu);
    UiPromptClose(&s->prompt);
    ShopClose(&s->shop);
    ShopRestock(&s->shop);
    UiDialogHide(&s->dialog);
    s->after_script = AFTER_NONE;

    if (s->phase == PHASE_SETTLED) PlayIntro(s, def);

    TraceLog(LOG_INFO, "SCENE: loaded %d (actors=%d shop=%d settle=%.2fs)",
             id, (int)s->actors, (int)def->has_shop, SettleTime(def));
}

static void BeginTravel(Scene *s, int to)
{
    if (to < 0 || to >= SCENE_COUNT || to == (int)s->id) return;
    s->fade_dir = +1;
    s->fade_target = (unsigned char)to;
    UiMenuClose(&s->menu);
    ShopClose(&s->shop);
    UiDialogHide(&s->dialog);
    TraceLog(LOG_INFO, "SCENE: travel %d -> %d", s->id, to);
}

void SceneInit(Scene *s, SceneId start)
{
    UiDialogInit(&s->dialog);
    UiMenuInit(&s->menu);
    ShopInit(&s->shop);
    s->prompt.open = false;
    s->prompt_kind = PROMPT_SYSTEM;
    s->after_script = AFTER_NONE;
    s->quit_requested = false;
    s->fade = 0.0f;
    s->fade_dir = 0;
    s->fade_target = 0;
    s->actors = 0;
    InvReset();
    LoadRoom(s, start);
}

bool SceneLoadSave(Scene *s)
{
    SaveBlob blob;
    if (!ReadSave(&blob)) return false;

    for (int i = 0; i < ITEM_COUNT; i++) g_inv.held[i] = blob.held[i];
    g_inv.gold = blob.gold;

    UnloadRoom(s);
    LoadRoom(s, (SceneId)blob.scene);

    /* The entrance and the welcome are skipped. A save is resumed, not
       replayed: watching two men walk in again every time you load is the
       kind of thing that makes people stop saving. The room's script counter
       is not restored either - the blob records where the player was, and a
       half-finished conversation is not a place. */
    s->clock = SettleTime(&SCENES[blob.scene]);
    for (int i = 0; i < s->actors; i++) {
        s->actor_x[i] = (float)SCENES[blob.scene].actor[i].rest_x;
    }
    s->phase = PHASE_DONE;
    UiDialogHide(&s->dialog);
    s->after_script = AFTER_NONE;

    TraceLog(LOG_INFO, "SCENE: loaded save (room %d, %d gold)",
             blob.scene, blob.gold);
    return true;
}

void SceneReset(Scene *s)
{
    const SceneId id = (SceneId)s->id;
    UnloadRoom(s);
    s->quit_requested = false;
    s->fade = 0.0f;
    s->fade_dir = 0;
    LoadRoom(s, id);
}

/* ---- update ------------------------------------------------------------ */

/* Ease-out cubic: enters at full speed, decelerates into place. Reads as a
   character walking on and stopping rather than a UI panel sliding. */
static float EaseOutCubic(float t)
{
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

static void UpdateFade(Scene *s, float dt)
{
    if (s->fade_dir == 0) return;

    s->fade += (float)s->fade_dir * (dt / FADE_SECONDS);

    if (s->fade_dir > 0 && s->fade >= 1.0f) {
        s->fade = 1.0f;
        UnloadRoom(s);
        LoadRoom(s, (SceneId)s->fade_target);
        s->fade_dir = -1;
    } else if (s->fade_dir < 0 && s->fade <= 0.0f) {
        s->fade = 0.0f;
        s->fade_dir = 0;
    }
}

void SceneUpdate(Scene *s, float dt)
{
    UpdateFade(s, dt);
    ShopUpdate(&s->shop, dt);

    const SceneDef *def = &SCENES[s->id];
    s->clock += dt;

    /* Each actor walks on against the same room clock, offset by its own
       delay. One clock rather than one timer per actor: a shared clock is
       what makes "JACK arrives 2.5 s after BEST" a number in the table
       instead of a chain of callbacks. */
    for (int i = 0; i < s->actors; i++) {
        const SceneActor *a = &def->actor[i];
        const float from = StartX(a);

        float t = (s->clock - a->delay) / ENTER_SECONDS;
        if (t <= 0.0f) { s->actor_x[i] = from; continue; }
        if (t >= 1.0f) { s->actor_x[i] = (float)a->rest_x; continue; }

        s->actor_x[i] = from + ((float)a->rest_x - from) * EaseOutCubic(t);
    }

    if (s->phase == PHASE_ENTER && s->clock >= SettleTime(def)) {
        s->phase = PHASE_SETTLED;
        PlayIntro(s, def);
    }

    UiDialogUpdate(&s->dialog, dt);
}

bool SceneWantsQuit(const Scene *s) { return s->quit_requested; }

/* ---- input -------------------------------------------------------------

   Every entry point below routes to exactly one layer, topmost first:
   prompt (modal) > shop > menu > dialogue > room. Nothing falls through, so
   a key press can never mean two things at once. */

static bool Busy(const Scene *s) { return s->fade_dir != 0; }

void SceneToggleMenu(Scene *s)
{
    if (Busy(s) || UiPromptIsOpen(&s->prompt) || ShopIsOpen(&s->shop)) return;
    if (UiMenuIsOpen(&s->menu)) UiMenuClose(&s->menu);
    else if (s->dialog.phase == DIALOG_HIDDEN) UiMenuOpen(&s->menu);
}

void SceneMove(Scene *s, int dx, int dy)
{
    if (Busy(s)) return;
    if (UiPromptIsOpen(&s->prompt))    UiPromptMove(&s->prompt, dx);
    else if (ShopIsOpen(&s->shop))     ShopInput(&s->shop, dx, dy, false);
    else if (UiMenuIsOpen(&s->menu))   UiMenuInput(&s->menu, dx, dy, false, false);
}

void SceneSort(Scene *s, SortMode mode)
{
    /* Same routing rule as every other key: topmost layer only. The prompt is
       modal, so nothing behind it reorders while a question is on screen. */
    if (Busy(s) || UiPromptIsOpen(&s->prompt)) return;
    if (ShopIsOpen(&s->shop))            ShopSort(&s->shop, mode);
    else if (UiMenuTakesSort(&s->menu))  SortPress(&s->menu.sort, mode);
}

void SceneBack(Scene *s)
{
    /* ESC pops exactly one level of whatever is on top, and when nothing is
       open it raises the system prompt. ESC never ends the process itself:
       quitting is a deliberate confirmation, not a reflex. */
    if (Busy(s)) return;

    if (UiPromptIsOpen(&s->prompt)) {
        UiPromptClose(&s->prompt);
    } else if (ShopIsOpen(&s->shop)) {
        /* ESC at the counter hands the question to JACK rather than throwing
           a bare modal. 1.3 put up an END TRADING? box; the shopkeeper asking
           out loud is the same confirmation and one fewer piece of furniture,
           and it is the only place in the game where a UI decision is voiced
           by a character. */
        ShopClose(&s->shop);
        PlayScript(s, SHOP_AGAIN, LEN(SHOP_AGAIN), AFTER_ASK_AGAIN);
    } else if (UiMenuIsOpen(&s->menu)) {
        UiMenuInput(&s->menu, 0, 0, false, true);
    } else if (s->dialog.phase != DIALOG_HIDDEN) {
        UiDialogHide(&s->dialog);
    } else {
        OpenSystemPrompt(s);
    }
}

void SceneAdvance(Scene *s)
{
    if (Busy(s)) return;

    if (UiPromptIsOpen(&s->prompt)) {
        const int choice = UiPromptAccept(&s->prompt);

        if (s->prompt_kind == PROMPT_ANYTHING_ELSE) {
            if (choice == AGAIN_YES) {
                ShopResume(&s->shop);
            } else {
                PlayScript(s, SHOP_BYE, LEN(SHOP_BYE), AFTER_LEAVE_SHOP);
            }
            return;
        }

        switch (choice) {
        case SYS_SAVE: {
            /* Re-opened so the result lands where the choice was made, rather
               than as a toast the player may already have looked away from.
               Written once and the result reused: calling WriteSave twice to
               log it would have doubled the disk traffic. */
            const bool ok = WriteSave(s);
            OpenSystemPrompt(s);
            s->prompt.note = ok ? "Progress saved." : "Save failed.";
            TraceLog(LOG_INFO, "SCENE: save -> %s", ok ? "ok" : "FAILED");
        } break;
        case SYS_QUIT:
            s->quit_requested = true;
            break;
        default:
            break;
        }
        return;
    }

    if (ShopIsOpen(&s->shop)) {
        ShopInput(&s->shop, 0, 0, true);
        return;
    }

    if (UiMenuIsOpen(&s->menu)) {
        const MenuCommand cmd = UiMenuInput(&s->menu, 0, 0, true, false);
        if (cmd == MENU_CMD_TALK) {
            const SceneDef *def = &SCENES[s->id];
            /* In a room with a counter, talking to the shopkeeper is how you
               reach it. He speaks first and the counter follows, because a
               trader who opens his ledger without a word is a vending
               machine. */
            PlayScript(s, def->solo, def->solo_len,
                       def->has_shop ? AFTER_OPEN_SHOP : AFTER_NONE);
        } else if (MenuCmdIsTravel(cmd)) {
            BeginTravel(s, MenuCmdScene(cmd));
        }
        return;
    }

    if (s->phase == PHASE_ENTER) {
        /* Impatient player: everyone snaps to their mark and the script
           starts. Skipping has to land the whole room, not just whoever
           happened to be walking when the key was pressed. */
        const SceneDef *def = &SCENES[s->id];
        s->clock = SettleTime(def);
        for (int i = 0; i < s->actors; i++) {
            s->actor_x[i] = (float)def->actor[i].rest_x;
        }
        s->phase = PHASE_SETTLED;
        PlayIntro(s, def);
        return;
    }

    if (s->dialog.phase == DIALOG_HIDDEN) {
        /* Script over in a shop room: the counter is what the player is here
           for, so the accept key opens it rather than doing nothing. */
        if (SCENES[s->id].has_shop && s->phase == PHASE_DONE) {
            ShopOpen(&s->shop);
        }
        return;
    }

    if (UiDialogAdvance(&s->dialog)) {
        s->script_at++;
        if (s->script_at < s->script_len) {
            UiDialogShow(&s->dialog, &s->script[s->script_at]);
            return;
        }
        if (s->phase == PHASE_SETTLED) s->phase = PHASE_DONE;

        const AfterScript then = (AfterScript)s->after_script;
        s->after_script = AFTER_NONE;
        switch (then) {
        case AFTER_OPEN_SHOP:   ShopOpen(&s->shop);   break;
        case AFTER_RESUME_SHOP: ShopResume(&s->shop); break;
        case AFTER_ASK_AGAIN:
            s->prompt_kind = PROMPT_ANYTHING_ELSE;
            UiPromptOpen(&s->prompt, "ANYTHING ELSE?", AGAIN_OPTIONS, 2,
                         AGAIN_YES);
            break;
        case AFTER_LEAVE_SHOP:
            UiDialogHide(&s->dialog);
            BeginTravel(s, SCENE_SMITHY);
            break;
        default: break;
        }
    }
}

/* ---- draw -------------------------------------------------------------- */

void SceneDraw(const Scene *s)
{
    DrawTexture(s->bg, 0, 0, WHITE);

    /* Snapped to whole virtual pixels: with point filtering a fractional
       offset makes the sprite shimmer against the backdrop's pixel grid.

       Drawn in table order, so an actor listed later overlaps one listed
       earlier. In the shop that is JACK over the counter he steps out from
       behind; the table is the depth order. */
    for (int i = 0; i < s->actors; i++) {
        DrawTexture(s->actor[i], (int)floorf(s->actor_x[i] + 0.5f),
                    (int)SCENES[s->id].actor[i].rest_y, WHITE);
    }

    UiDialogDraw(&s->dialog);
    ShopDraw(&s->shop);
    UiMenuDraw(&s->menu);
    UiPromptDraw(&s->prompt);

    if (s->fade > 0.0f) {
        const unsigned char a = (unsigned char)(s->fade * 255.0f);
        DrawRectangle(0, 0, VSCREEN_W, VSCREEN_H, (Color){ 0, 0, 0, a });
    }
}

void SceneUnload(Scene *s)
{
    UiDialogUnload(&s->dialog);
    UnloadRoom(s);
}
