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
/* Said to nobody in particular, which is why there is no speaker and no
   portrait: it is the day turning over, not a character talking. */
static const DialogLine DAY_BREAK[] = {
{ NULL, "The coals are banked and the shutters\nare down. Tomorrow, then.", MOOD_TIRED, PORTRAIT_HERO },
};

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
        .feature = ROOM_FEATURE_FORGE,
        /* Measured off resource/BG-01-SMITTY-A03.png rather than guessed:
           a threshold pass, connected-component labelled to tell the hearth
           from the lantern and the window, puts the lit mouth at x 216..247,
           y 110..131. So the glow centres on (232,121) and the coal bed sits
           nine pixels under that. Numbers taken from the art are numbers that
           stay right when the art is re-exported at a different crop. */
        .lights = 1,
        .light = {
            /* rx/ry are the full reach now, not the core: 1.9 drew a core
               blob plus a halo at twice the radius, and 1.9.3's stacked
               falloff replaces both, so the numbers are the old outer pair. */
            { .x = 232, .y = 121, .rx = 30, .ry = 21, .kind = LIGHT_HEARTH,
              .embers = 14, .bed_w = 26, .bed_dy = 9, .rise = 30 },
        },
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
        .feature = ROOM_FEATURE_TRADE,
        /* Same method on resource/BG-01-ITEM_SHOP-A01.png, and it needed
           more care than the smithy did. The first pass ranked by luminance
           and found the doorway and the skylight, which are far brighter than
           a wick and are not lamps. Warmth over luminance, inside windows
           chosen by eye, finds the three that are.

           1.9.2 re-measured them properly. 1.9.1 took the single brightest
           warm cell in each window, which is one sample and lands wherever
           the resample happened to put a highlight; these are warm-weighted
           centroids over the whole core, which is the actual centre of the
           light:

               wall lantern     (12.8, 62.6)   over a core x 4..21, y 53..73
               hanging lamp A  (224.0, 29.0)   over x 217..231, y 26..32
               hanging lamp B  (252.2, 29.4)   over x 248..255, y 27..33

           Lamp B moved a pixel left and a pixel down. Two of the three were
           already right, which is the useful part of the result: the cheap
           method was not wrong so much as unverifiable.

           None of them sparks. A lamp that threw embers would be a lamp about
           to set fire to a shop full of potions.

           1.9.3 sized the haloes to what a lamp actually throws. They were
           rx 4..5, which is the lit glass and not the light: at 320x240 that
           is a bright dot sitting on a lamp, and the eye reads it as part of
           the painting rather than as something happening. Round, and out to
           roughly the reach a wick lights - the lantern is the biggest fixture
           so it carries the widest. rx equals ry: these are circles, and the
           ellipse is reserved for the hearth mouth, which genuinely is one. */
        .lights = 3,
        .light = {
            { .x =  13, .y = 63, .rx = 13, .ry = 13, .kind = LIGHT_LAMP },
            { .x = 224, .y = 29, .rx = 11, .ry = 11, .kind = LIGHT_LAMP },
            { .x = 252, .y = 29, .rx = 11, .ry = 11, .kind = LIGHT_LAMP },
        },
        .intro = SHOP_INTRO, .intro_len = LEN(SHOP_INTRO),
        .solo  = SHOP_SOLO,  .solo_len  = LEN(SHOP_SOLO),
    },
};

/* ---- system prompt ----------------------------------------------------- */

/* CANCEL is first and is the default, so a stray ESC followed by a stray
   confirm is harmless. */
/* 1.8 reordered and stacked these. The row put CANCEL first because it is
   the safe answer and the safe answer goes under the cursor - but reading
   order and cursor position are different jobs, and a row made them fight.
   Stacked, the list reads in the order the actions escalate, SAVE then
   CANCEL then QUIT, and CANCEL still starts under the cursor by sitting in
   the middle. Nothing destructive is one keypress from open.

   A column rather than a row because these three are unlike each other. A row
   invites the eye to scan a spectrum, which is right for NOT YET / END DAY
   and wrong for a menu of unrelated verbs. */
static const char *const SYSTEM_OPTIONS[3] = { "SAVE", "CANCEL", "QUIT" };
#define SYS_SAVE   0
#define SYS_CANCEL 1
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

/* The smithy's two doors. A UiPrompt rather than a bespoke two-row menu:
   this is the same shape as every other question the game asks, and the
   widget was written generically in 1.1 for exactly this. A hand-rolled
   chooser would have been ~300 bytes to say what a title and two labels
   already say. */
static const char *const SMITHY_OPTIONS[2] = { "FORGE", "BLUEPRINTS" };
#define SMITHY_FORGE_OPT 0
#define SMITHY_BOOK_OPT  1

/* Ending the day is the one irreversible thing in the menu, so it asks. NO is
   the default: a confirmation whose default is yes is a slower way of not
   asking. */
static const char *const END_DAY_OPTIONS[2] = { "NOT YET", "END DAY" };
#define END_DAY_NO  0
#define END_DAY_YES 1

typedef enum PromptKind {
    PROMPT_SYSTEM = 0,
    PROMPT_ANYTHING_ELSE,
    PROMPT_SMITHY,
    PROMPT_END_DAY
} PromptKind;

/* What a finished script hands control to. */
typedef enum AfterScript {
    AFTER_NONE = 0,
    /* AFTER_OPEN_SHOP is gone with 1.7. Nothing opens a counter on its own
       any more; the only way in is the menu row. */
    AFTER_RESUME_SHOP,   /* the same counter, state intact       */
    AFTER_ASK_AGAIN,     /* raise the "anything else?" prompt    */
    AFTER_LEAVE_SHOP     /* walk out of the room                 */
} AfterScript;

/* Raised by the FORGE row of the main menu. Blueprints is a door off the
   anvil rather than a menu row of its own: the root grid is what the room
   lets you do, and reading about a recipe is part of forging, not a second
   thing the smithy is for. */
static void OpenSmithyPrompt(Scene *s)
{
    s->prompt_kind = PROMPT_SMITHY;
    UiPromptOpen(&s->prompt, "THE ANVIL", SMITHY_OPTIONS, 2, SMITHY_FORGE_OPT);
}

/* Starting a heat. The ore leaves the pack here rather than when the minigame
   ends, which is the whole reason InvForge was split: a ruined heat has to
   cost something, and the only way it can is if the cost was already paid.

   The list closes rather than drawing behind the minigame. The QTE washes the
   frame like any other modal, and a list legible under the wash would invite
   the player to read it while a timer they cannot see is running. */
static void BeginHeat(Scene *s, int recipe)
{
    if (!InvSpendMaterials(recipe)) return;
    ForgeClose(&s->forge);
    QteBegin(&s->qte, recipe);
    TraceLog(LOG_INFO, "FORGE: heat begins on recipe %d", recipe);
}

/* Collecting the verdict. The grant happens here and not in qte.c, because
   the minigame's job ends at a grade - it does not know what a pack is. */
static void EndHeat(Scene *s)
{
    const int recipe = s->qte.recipe;
    const QteGrade grade = QteResult(&s->qte);
    const int out = QteOutput(recipe, grade);

    QteClose(&s->qte);

    const char *line;
    if (out < 0) {
        line = "The metal is spoiled.";
    } else if (InvGrantItem(out)) {
        line = (grade == QTE_FINE) ? "A fine piece." : "That will sell.";
    } else {
        line = "No room in the pack.";
    }

    ForgeResume(&s->forge, line);
    TraceLog(LOG_INFO, "FORGE: grade %d -> item %d", (int)grade, out);
}

/* The room's own menu row. The scene owns this switch because the scene owns
   the world; ui_menu.c only knows that the room declared a feature and what
   it is called. */
static void OpenRoomFeature(Scene *s)
{
    switch (SCENES[s->id].feature) {
    case ROOM_FEATURE_TRADE: ShopOpen(&s->shop); break;
    case ROOM_FEATURE_FORGE: OpenSmithyPrompt(s); break;
    /* ROOM_FEATURE_PARTY has no room to be reached from yet. It is listed so
       that the day the Guild gets a backdrop, this is not the file that has
       to be found and edited. */
    default: break;
    }
}

/* Notes are budgeted where they are written, not where they are drawn. 1.7
   set this one inline and it drew 26 px out through each wall of a box that
   was 152 px wide no matter what went in it. The box measures itself now, and
   these assertions catch the string that would be too wide even for that. */
#define NOTE_END_DAY "The forge goes cold until morning."
#define NOTE_SAVED   "Progress saved."
#define NOTE_SAVE_NO "Save failed."
UI_PROMPT_FITS(NOTE_END_DAY);
UI_PROMPT_FITS(NOTE_SAVED);
UI_PROMPT_FITS(NOTE_SAVE_NO);

static void OpenEndDayPrompt(Scene *s)
{
    s->prompt_kind = PROMPT_END_DAY;
    UiPromptOpen(&s->prompt, "END THE DAY?", END_DAY_OPTIONS, 2, END_DAY_NO);
    s->prompt.note = NOTE_END_DAY;
}

/* Turning the day over. The fade is the same one travel uses, with a flag
   saying what the black is hiding, so the two transitions cannot drift apart
   in length or curve. */
static void BeginDayChange(Scene *s)
{
    s->fade_dir = +1;
    s->fade_kind = FADE_DAY;
    UiMenuClose(&s->menu);
    ShopClose(&s->shop);
    ForgeClose(&s->forge);
    UiDialogHide(&s->dialog);
}

/* Called at full black. Everything that happens overnight happens here, which
   is the point of having a day boundary at all: it is the one moment the
   world is allowed to change without the player watching. */
static void AdvanceDay(Scene *s)
{
    if (s->day < 0xFFFFu) s->day++;
    ShopRestock(&s->shop);
    TraceLog(LOG_INFO, "SCENE: day %d begins", (int)s->day);
}

static void OpenSystemPrompt(Scene *s)
{
    s->prompt_kind = PROMPT_SYSTEM;
    UiPromptOpenColumn(&s->prompt, "PAUSED", SYSTEM_OPTIONS, 3, SYS_CANCEL);
}

/* ---- save -------------------------------------------------------------- */

/* Versioned from the first byte so an old save is recognised and rejected
   rather than misread. 1.2 adds gold, the room the player is standing in and
   the held counts, so the version goes to 2 and a v1 blob is refused: the
   fields it lacks have no safe default now that coin can be spent. */
#define SAVE_VERSION 4
#define SAVE_PATH "iron.sav"

typedef struct SaveBlob {
    unsigned char magic[4];        /* 'I','R','O','N' */
    unsigned short version;
    unsigned short scene;
    unsigned short phase;
    unsigned short script_at;
    int gold;
    unsigned short held[ITEM_COUNT];
    /* 1.5. Which recipes have been learned. The blob also grew by three held
       counts when the forge's output items were added, so a v2 save is the
       wrong length as well as the wrong version and is refused twice over. */
    unsigned int known;
    /* 1.7. Which day it is. A save that restored the pack but not the date
       would put the player back on day one with a week of stock spent. */
    unsigned short day;
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
        (unsigned short)s->script_at, InvGold(), { 0 }, g_inv.known, s->day
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

/* 1.4 had the welcome lead straight into the counter, and 1.5 gave the anvil
   the same courtesy from the accept key. 1.7 took both away. Every feature is
   reached from the main menu now, and a room that also opens its own counter
   teaches the player that some things are in the menu and some things happen
   on their own - which is exactly the confusion the single entry point exists
   to remove. The M hint in the corner is what replaces it. */
static void PlayIntro(Scene *s, const SceneDef *def)
{
    PlayScript(s, def->intro, def->intro_len, AFTER_NONE);
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
    ForgeClose(&s->forge);
    QteClose(&s->qte);
    VfxRoomStart(&s->ambience, def->light, (int)def->lights);
    UiDialogHide(&s->dialog);
    s->after_script = AFTER_NONE;

    if (s->phase == PHASE_SETTLED) PlayIntro(s, def);

    TraceLog(LOG_INFO, "SCENE: loaded %d (actors=%d shop=%d settle=%.2fs)",
             id, (int)s->actors, (int)def->feature, SettleTime(def));
}

static void BeginTravel(Scene *s, int to)
{
    if (to < 0 || to >= SCENE_COUNT || to == (int)s->id) return;
    s->fade_dir = +1;
    s->fade_kind = FADE_TRAVEL;
    s->fade_target = (unsigned char)to;
    UiMenuClose(&s->menu);
    ShopClose(&s->shop);
    ForgeClose(&s->forge);
    QteClose(&s->qte);
    UiDialogHide(&s->dialog);
    TraceLog(LOG_INFO, "SCENE: travel %d -> %d", s->id, to);
}

void SceneInit(Scene *s, SceneId start)
{
    UiDialogInit(&s->dialog);
    UiMenuInit(&s->menu);
    ShopInit(&s->shop);
    ForgeInit(&s->forge);
    QteInit(&s->qte);
    VfxRoomStop(&s->ambience);
    s->prompt.open = false;
    s->prompt_kind = PROMPT_SYSTEM;
    s->after_script = AFTER_NONE;
    s->quit_requested = false;
    s->fade = 0.0f;
    s->fade_dir = 0;
    s->fade_kind = FADE_TRAVEL;
    s->fade_target = 0;
    s->day = 1;
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
    g_inv.known = blob.known;
    s->day = blob.day ? blob.day : 1;

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
    s->day = 1;
    const SceneId id = (SceneId)s->id;
    UnloadRoom(s);
    s->quit_requested = false;
    s->fade = 0.0f;
    s->fade_dir = 0;
    s->fade_kind = FADE_TRAVEL;
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
        if (s->fade_kind == FADE_DAY) {
            AdvanceDay(s);
            s->fade_dir = -1;
            /* Said on the way back up rather than at full black, so the line
               is readable rather than a flash behind the curtain. */
            PlayScript(s, DAY_BREAK, LEN(DAY_BREAK), AFTER_NONE);
            return;
        }
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
    ForgeUpdate(&s->forge, dt);
    QteUpdate(&s->qte, dt);
    /* Ambience keeps running behind a panel. A hearth that freezes the moment
       the player opens a menu is worse than no hearth at all. */
    VfxRoomUpdate(&s->ambience, dt);
    if (QteFinished(&s->qte)) EndHeat(s);

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
    if (Busy(s) || QteIsOpen(&s->qte) || UiPromptIsOpen(&s->prompt) ||
        ShopIsOpen(&s->shop) || ForgeIsOpen(&s->forge)) return;
    if (UiMenuIsOpen(&s->menu)) UiMenuClose(&s->menu);
    else if (s->dialog.phase == DIALOG_HIDDEN) {
        UiMenuOpen(&s->menu, (int)SCENES[s->id].feature);
    }
}

void SceneMove(Scene *s, int dx, int dy)
{
    if (Busy(s)) return;
    /* The minigame takes the arrow keys before anything else can. dx/dy are
       translated into QteKey here so qte.c never learns what a keyboard is,
       and main.c needs no new bindings at all. */
    if (QteIsOpen(&s->qte)) {
        if (dx < 0)      QteKeyPress(&s->qte, QK_LEFT);
        else if (dx > 0) QteKeyPress(&s->qte, QK_RIGHT);
        else if (dy < 0) QteKeyPress(&s->qte, QK_UP);
        else if (dy > 0) QteKeyPress(&s->qte, QK_DOWN);
        return;
    }
    if (UiPromptIsOpen(&s->prompt))    UiPromptMove(&s->prompt, dx, dy);
    else if (ShopIsOpen(&s->shop))     ShopInput(&s->shop, dx, dy, false);
    else if (ForgeIsOpen(&s->forge))   ForgeInput(&s->forge, dx, dy, false);
    else if (UiMenuIsOpen(&s->menu))   UiMenuInput(&s->menu, dx, dy, false, false);
}

void SceneSort(Scene *s, SortMode mode)
{
    /* Same routing rule as every other key: topmost layer only. The prompt is
       modal, so nothing behind it reorders while a question is on screen. */
    if (Busy(s) || QteIsOpen(&s->qte) || UiPromptIsOpen(&s->prompt)) return;
    if (ShopIsOpen(&s->shop))            ShopSort(&s->shop, mode);
    else if (ForgeIsOpen(&s->forge))     ForgeSort(&s->forge, mode);
    else if (UiMenuTakesSort(&s->menu))  SortPress(&s->menu.sort, mode);
}

void SceneBack(Scene *s)
{
    /* ESC pops exactly one level of whatever is on top, and when nothing is
       open it raises the system prompt. ESC never ends the process itself:
       quitting is a deliberate confirmation, not a reflex. */
    if (Busy(s)) return;

    /* The one place ESC does nothing. The ore is already in the fire, the
       sequence always terminates on its own, and a reflexive back-key that
       threw a heat away would be exactly what the ladder exists to prevent. */
    if (QteIsOpen(&s->qte)) return;

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
    } else if (ForgeIsOpen(&s->forge)) {
        /* One level, like everywhere else: the list closes and the question
           that opened it comes back, so a player who wanted the other door
           does not have to walk back to the anvil to find it. */
        ForgeClose(&s->forge);
        OpenSmithyPrompt(s);
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

    if (QteIsOpen(&s->qte)) { QteKeyPress(&s->qte, QK_SPACE); return; }

    if (UiPromptIsOpen(&s->prompt)) {
        const int choice = UiPromptAccept(&s->prompt);

        if (s->prompt_kind == PROMPT_SMITHY) {
            if (choice == SMITHY_FORGE_OPT) ForgeOpen(&s->forge, FORGE_MODE_CRAFT);
            else if (choice == SMITHY_BOOK_OPT) ForgeOpen(&s->forge, FORGE_MODE_BOOK);
            return;
        }

        if (s->prompt_kind == PROMPT_END_DAY) {
            if (choice == END_DAY_YES) BeginDayChange(s);
            /* NOT YET leaves the menu exactly as it was, still open behind
               the box the player just dismissed. */
            return;
        }

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
            s->prompt.note = ok ? NOTE_SAVED : NOTE_SAVE_NO;
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

    if (ForgeIsOpen(&s->forge)) {
        const ForgeCommand cmd = ForgeInput(&s->forge, 0, 0, true);
        if (ForgeCmdIsBegin(cmd)) BeginHeat(s, ForgeCmdRecipe(cmd));
        return;
    }

    if (UiMenuIsOpen(&s->menu)) {
        const MenuCommand cmd = UiMenuInput(&s->menu, 0, 0, true, false);
        if (cmd == MENU_CMD_TALK) {
            const SceneDef *def = &SCENES[s->id];
            /* TALK is now only talk. Trading is BUY/SELL, one row down. */
            PlayScript(s, def->solo, def->solo_len, AFTER_NONE);
        } else if (cmd == MENU_CMD_FEATURE) {
            OpenRoomFeature(s);
        } else if (cmd == MENU_CMD_END_DAY) {
            OpenEndDayPrompt(s);
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

    /* Nothing to advance and nothing to open: the accept key on an idle room
       does nothing at all, and the corner hint says where to go instead. */
    if (s->dialog.phase == DIALOG_HIDDEN) return;

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

/* ---- the standing HUD --------------------------------------------------

   Two pieces, both outside every panel in the game, both drawn last.

   The date badge sits in the 8 px margin above the page frame - PAGE_Y is 8,
   the badge is 8 tall, so it never overlaps a screen and never has to be
   suppressed for one. That is what lets it be on screen at all times without
   a single special case.

   The menu hint is the answer to the question 1.7 created. Taking away the
   counter that opened itself and the anvil that answered the accept key left
   a room that does nothing when you press anything, so the way in has to be
   written down somewhere. It shows only when the room is idle: with a panel
   up the player has already found the menu, and with dialogue up the balloon
   is where the bottom of the screen is being read. */

/* 1.8 gave the badge a frame. A bare filled rectangle was legible over the
   smithy's dark beams and vanished into the shop's lit shelves; every other
   piece of standing furniture in the game is a UiPanel and this was the one
   thing floating.

   The height is the tight part. A bordered panel needs a border row, a pad
   row, seven rows of glyph, a pad and a border - eleven - and it has to clear
   the page title line at PAGE_Y + 7, because that is where the shop draws its
   purse. Twelve fits with a pixel to spare and the assertion below is what
   keeps it fitting; it is the only reason the badge can be on screen at all
   times without a single per-screen special case. It does sit *on* the page
   frame's top edge rather than above it, which is what the mock shows. */
/* 1.9 widened it. 1.8 sized the box to the tightest thing that would hold
   five digits, which is how you get a box that is correct and looks mean -
   64x12 with five pixels of air either side reads as a label crammed into
   its own frame rather than a plate with something on it.

   The height is capped by the assertion below and nothing else, so it takes
   all of it. The width is free, so the padding and the gap between DAY and
   the number both roughly doubled: the number now has somewhere to sit
   rather than being pushed against the right wall. */
#define HUD_DAY_Y     1
#define HUD_DAY_H    13
#define HUD_DAY_PAD   9
#define HUD_DAY_GAP  16
#define HUD_DAY_DIGITS 5
#define HUD_DAY_W  (HUD_DAY_PAD * 2 + FONT_CELL_W * 3 + HUD_DAY_GAP + \
                    FONT_CELL_W * HUD_DAY_DIGITS)
#define HUD_DAY_X  (VSCREEN_W - HUD_DAY_W - 5)

_Static_assert(HUD_DAY_Y + HUD_DAY_H <= PAGE_Y + 7,
               "the date badge would land on a page's title line");

#define HUD_HINT      "M  MENU"
#define HUD_HINT_X    8
#define HUD_HINT_Y  (VSCREEN_H - 10)

/* The build stamp, opposite the menu hint on the same row. Dim, small, and
   never suppressed: a screenshot or a stream of a contest build should say
   which build it is without anyone having to ask, and the moment it can be
   hidden is the moment the one screenshot that matters was taken with it
   hidden. It sits on the page frame's bottom edge for the same reason the
   date badge sits on the top one. */
#define HUD_VER_X   (VSCREEN_W - 6)
#define HUD_VER_Y   HUD_HINT_Y

_Static_assert(sizeof(HUD_HINT) - 1 + sizeof(GAME_VERSION) < 30,
               "the menu hint and the version stamp would meet in the middle");

static void DrawDayBadge(const Scene *s)
{
    /* Fixed width rather than measured, so the box does not twitch a glyph
       wider on the day the counter reaches ten. */
    UiPanel(HUD_DAY_X, HUD_DAY_Y, HUD_DAY_W, HUD_DAY_H, UI_FILL, UI_EDGE);
    UiDrawText("DAY", HUD_DAY_X + HUD_DAY_PAD, HUD_DAY_Y + 3, UI_DIM);
    UiNumber(HUD_DAY_X + HUD_DAY_W - HUD_DAY_PAD, HUD_DAY_Y + 3,
             (int)s->day, UI_TEXT);
}

static bool RoomIsIdle(const Scene *s)
{
    return !UiPromptIsOpen(&s->prompt) && !ShopIsOpen(&s->shop) &&
           !ForgeIsOpen(&s->forge) && !QteIsOpen(&s->qte) &&
           !UiMenuIsOpen(&s->menu) && s->dialog.phase == DIALOG_HIDDEN &&
           s->fade_dir == 0;
}

void SceneDraw(const Scene *s)
{
    DrawTexture(s->bg, 0, 0, WHITE);

    /* Snapped to whole virtual pixels: with point filtering a fractional
       offset makes the sprite shimmer against the backdrop's pixel grid.

       Drawn in table order, so an actor listed later overlaps one listed
       earlier. In the shop that is JACK over the counter he steps out from
       behind; the table is the depth order. */
    /* Between the backdrop and the cast: in the smithy BEST stands at x=239
       and the hearth is at 216..248, so he has to occlude the near edge of
       it. Behind the cast is also right for the shop's lamps, which hang
       further back in the room than either figure stands. */
    VfxRoomDraw(&s->ambience);

    for (int i = 0; i < s->actors; i++) {
        DrawTexture(s->actor[i], (int)floorf(s->actor_x[i] + 0.5f),
                    (int)SCENES[s->id].actor[i].rest_y, WHITE);
    }

    UiDialogDraw(&s->dialog);
    ShopDraw(&s->shop);
    ForgeDraw(&s->forge);
    QteDraw(&s->qte);
    UiMenuDraw(&s->menu);
    UiPromptDraw(&s->prompt);

    if (RoomIsIdle(s)) UiDrawText(HUD_HINT, HUD_HINT_X, HUD_HINT_Y, UI_DIM);
    UiDrawText(GAME_VERSION,
               HUD_VER_X - UiTextWidth(GAME_VERSION), HUD_VER_Y, UI_DIM);
    DrawDayBadge(s);

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
