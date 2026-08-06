#include "scene_smithy.h"

#include <math.h>

#include "../assets/bg_smithy.h"
#include "../assets/hero_idle.h"

/* Placeholder script for the demo. Lines are hand-wrapped: the balloon holds
   39 columns and three rows, and a wrapper would cost code for no benefit
   while the script is authored by hand. */
static const DialogLine SMITHY_SCRIPT[] = {
    { "BEST",  "The forge is lit. Iron does not care\nwho is watching, only who is willing\nto swing.", MOOD_STERN },
    { "BEST",  "Every blade I sink coin into comes\nback with a story. Some of them come\nback with the party still breathing.", MOOD_SMIRK },
    { "BEST",  "So. Shall we find out what this one\nis worth?", MOOD_SMILE },
};
#define SMITHY_SCRIPT_LEN ((int)(sizeof(SMITHY_SCRIPT) / sizeof(SMITHY_SCRIPT[0])))

/* TALK with nobody in the room: the smith talks to himself. Selected by the
   command menu, which is why the scene owns it rather than the menu. */
static const DialogLine SOLO_SCRIPT[] = {
    { "BEST", "Talking to an empty forge again.\nThe coal never argues, at least.", MOOD_SIGH },
    { "BEST", "Thirty-four lumps of ore, eighteen\nsacks of charcoal, and one cracked\nanvil horn. That is the whole empire.", MOOD_GLUM },
    { "BEST", "Right. Enough. Something in here has\nto get hit today.", MOOD_SMIRK },
};
#define SOLO_SCRIPT_LEN ((int)(sizeof(SOLO_SCRIPT) / sizeof(SOLO_SCRIPT[0])))

/* System prompt. CANCEL is first and is the default, so a stray ESC followed
   by a stray confirm is harmless. */
static const char *const SYSTEM_OPTIONS[3] = { "CANCEL", "SAVE", "QUIT" };
#define SYS_CANCEL 0
#define SYS_SAVE   1
#define SYS_QUIT   2

/* On-disk state. Versioned from the first byte so an old save can be
   recognised and rejected rather than misread. */
typedef struct SaveBlob {
    unsigned char magic[4];   /* 'I','R','O','N' */
    unsigned short version;
    unsigned short phase;
    unsigned short script_at;
    unsigned short reserved;
} SaveBlob;

#define SAVE_VERSION 1
#define SAVE_PATH "iron.sav"

static bool WriteSave(const SceneSmithy *s)
{
    SaveBlob blob = {
        { 'I', 'R', 'O', 'N' }, SAVE_VERSION,
        (unsigned short)s->phase, (unsigned short)s->script_at, 0
    };
    return SaveFileData(SAVE_PATH, &blob, (int)sizeof(blob));
}

static void PlayScript(SceneSmithy *s, const DialogLine *lines, int len)
{
    s->script = lines;
    s->script_len = len;
    s->script_at = 0;
    UiDialogShow(&s->dialog, &lines[0]);
}

/* Layout, in virtual-screen pixels, matched to GAME-CONCEPT-SMITTY_HERO-A02.

   The sprite is 184 tall but only its top 167 rows are on screen: the bottom
   17 rows fall past y=240 and the screen clips them. That is deliberate.
   Cropping the legs lets the hero be drawn at a larger scale without breaking
   the room's perspective, because the framing reads as "close to camera"
   rather than "giant standing in the room". A full-height 184 sprite with its
   feet on the floor would look wrong; the same sprite cut at the shin does
   not.

   The full body is stored rather than a pre-cropped 167-row version. The
   extra 17 rows cost about 250 bytes and keep one asset usable for any future
   framing, instead of baking this shot into the data. */
#define HERO_REST_X   239.0f
#define HERO_REST_Y    73.0f
#define HERO_START_X  ((float)VSCREEN_W + 4.0f)   /* fully off the right edge */

#define HOLD_SECONDS  0.45f
#define ENTER_SECONDS 1.15f

/* Ease-out cubic: enters at full speed, decelerates into place. Reads as a
   character walking on and stopping rather than a UI panel sliding. */
static float EaseOutCubic(float t)
{
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

void SceneSmithyInit(SceneSmithy *s)
{
    s->bg = GfxLoadTexture(&bg_smithy);
    s->hero = GfxLoadTexture(&hero_idle);
    UiDialogInit(&s->dialog);
    UiMenuInit(&s->menu);
    s->prompt.open = false;
    SceneSmithyReset(s);
}

void SceneSmithyReset(SceneSmithy *s)
{
    s->phase = SMITHY_HOLD;
    s->clock = 0.0f;
    s->hero_x = HERO_START_X;
    s->script = SMITHY_SCRIPT;
    s->script_len = SMITHY_SCRIPT_LEN;
    s->script_at = 0;
    UiMenuClose(&s->menu);
    UiPromptClose(&s->prompt);
    s->quit_requested = false;
    UiDialogHide(&s->dialog);
    TraceLog(LOG_INFO, "SMITHY: reset -> HOLD (hold=%.2fs enter=%.2fs)",
             HOLD_SECONDS, ENTER_SECONDS);
}

void SceneSmithyUpdate(SceneSmithy *s, float dt)
{
    s->clock += dt;

    switch (s->phase) {
    case SMITHY_HOLD:
        if (s->clock >= HOLD_SECONDS) {
            s->clock -= HOLD_SECONDS;
            s->phase = SMITHY_ENTER;
            TraceLog(LOG_INFO, "SMITHY: HOLD -> ENTER");
        }
        break;

    case SMITHY_ENTER: {
        float t = s->clock / ENTER_SECONDS;
        if (t >= 1.0f) {
            t = 1.0f;
            s->phase = SMITHY_SETTLED;
        }
        const float k = EaseOutCubic(t);
        s->hero_x = HERO_START_X + (HERO_REST_X - HERO_START_X) * k;
        if (s->phase == SMITHY_SETTLED) {
            TraceLog(LOG_INFO, "SMITHY: ENTER -> SETTLED hero_x=%.0f", HERO_REST_X);
            PlayScript(s, SMITHY_SCRIPT, SMITHY_SCRIPT_LEN);
        }
    } break;

    case SMITHY_SETTLED:
    case SMITHY_DONE:
        s->hero_x = HERO_REST_X;
        break;
    }

    UiDialogUpdate(&s->dialog, dt);
}

bool SceneSmithyWantsQuit(const SceneSmithy *s)
{
    return s->quit_requested;
}

void SceneSmithyToggleMenu(SceneSmithy *s)
{
    if (UiPromptIsOpen(&s->prompt)) return;
    if (UiMenuIsOpen(&s->menu)) UiMenuClose(&s->menu);
    else if (s->dialog.phase == DIALOG_HIDDEN) UiMenuOpen(&s->menu);
}

void SceneSmithyMove(SceneSmithy *s, int dx, int dy)
{
    /* Input goes to the topmost layer only. The prompt is modal, so it wins
       outright; nothing behind it moves while it is up. */
    if (UiPromptIsOpen(&s->prompt)) UiPromptMove(&s->prompt, dx);
    else if (UiMenuIsOpen(&s->menu)) UiMenuInput(&s->menu, dx, dy, false, false);
}

void SceneSmithyBack(SceneSmithy *s)
{
    /* ESC pops exactly one level of whatever is on top, and when nothing is
       open it raises the system prompt. ESC never ends the process itself:
       quitting is a deliberate confirmation, not a reflex. */
    if (UiPromptIsOpen(&s->prompt)) {
        UiPromptClose(&s->prompt);
    } else if (UiMenuIsOpen(&s->menu)) {
        UiMenuInput(&s->menu, 0, 0, false, true);
    } else if (s->dialog.phase != DIALOG_HIDDEN) {
        UiDialogHide(&s->dialog);
    } else {
        UiPromptOpen(&s->prompt, "PAUSED", SYSTEM_OPTIONS, 3, SYS_CANCEL);
    }
}

void SceneSmithyAdvance(SceneSmithy *s)
{
    if (UiPromptIsOpen(&s->prompt)) {
        switch (UiPromptAccept(&s->prompt)) {
        case SYS_SAVE:
            /* Re-open so the result is visible where the choice was made,
               rather than as a toast the player may already have looked away
               from. */
            UiPromptOpen(&s->prompt, "PAUSED", SYSTEM_OPTIONS, 3, SYS_CANCEL);
            s->prompt.note = WriteSave(s) ? "Progress saved." : "Save failed.";
            TraceLog(LOG_INFO, "SMITHY: save -> %s",
                     WriteSave(s) ? "ok" : "FAILED");
            break;
        case SYS_QUIT:
            s->quit_requested = true;
            TraceLog(LOG_INFO, "SMITHY: quit requested");
            break;
        default:
            break;
        }
        return;
    }

    if (UiMenuIsOpen(&s->menu)) {
        if (UiMenuInput(&s->menu, 0, 0, true, false) == MENU_CMD_TALK) {
            PlayScript(s, SOLO_SCRIPT, SOLO_SCRIPT_LEN);
        }
        return;
    }

    if (s->phase == SMITHY_ENTER) {
        /* Impatient player: skip straight to the entrance's resting state. */
        s->phase = SMITHY_SETTLED;
        s->hero_x = HERO_REST_X;
        PlayScript(s, SMITHY_SCRIPT, SMITHY_SCRIPT_LEN);
        return;
    }

    if (s->dialog.phase == DIALOG_HIDDEN) return;

    if (UiDialogAdvance(&s->dialog)) {
        s->script_at++;
        if (s->script_at < s->script_len) {
            UiDialogShow(&s->dialog, &s->script[s->script_at]);
        } else if (s->phase == SMITHY_SETTLED) {
            s->phase = SMITHY_DONE;
            TraceLog(LOG_INFO, "SMITHY: script complete -> DONE (%d lines)", s->script_len);
        }
    }
}

void SceneSmithyDraw(const SceneSmithy *s)
{
    DrawTexture(s->bg, 0, 0, WHITE);

    /* Snapped to whole virtual pixels: with point filtering a fractional
       offset makes the sprite shimmer against the backdrop's pixel grid. */
    DrawTexture(s->hero, (int)floorf(s->hero_x + 0.5f), (int)HERO_REST_Y, WHITE);

    UiDialogDraw(&s->dialog);
    UiMenuDraw(&s->menu);
    UiPromptDraw(&s->prompt);
}

void SceneSmithyUnload(SceneSmithy *s)
{
    UiDialogUnload(&s->dialog);
    UnloadTexture(s->hero);
    UnloadTexture(s->bg);
}
