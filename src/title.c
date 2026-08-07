#include "title.h"

#include "scene.h"
#include "ui.h"
#include "ui_font.h"

#include "../assets/bg_smithy.h"

/* Every macro in this file is prefixed. The unity build puts ui_prompt.c and
   title.c in one translation unit, so a bare TITLE_Y here silently redefined
   the prompt's own - the compiler warned, but only because the values
   differed. Two screens that happened to agree on a number would have shared
   it without a word. */
#define T_SCRIM 175

#define T_HEAD_Y     52
#define T_SUB_Y       70
#define T_MENU_Y     116
#define T_ROW_H  18
#define T_MENU_W     120
#define T_MENU_H      15
#define T_NOTE_Y     198
#define T_FOOT_Y     222

#define T_BLINK 0.5f

static const char *const T_LABELS[TITLE_ROWS] = {
    "START", "LOAD", "OPTION", "EXIT"
};

#define T_NAME "IRON & INVESTMENT"
#define T_TAGLINE   "a blacksmith who backs adventurers"
#define T_FOOT       "ARROWS MOVE   SPACE SELECT"

/* Same budget as the page hints: this line sits at the same left margin. */
UI_HINT_FITS(T_FOOT);

void TitleLoad(UiTitle *t)
{
    t->bg = GfxLoadTexture(&bg_smithy);
    t->cursor = 0;
    t->note = NULL;
    t->clock = 0.0f;
    TitleRefresh(t);
}

void TitleUnload(UiTitle *t)
{
    UnloadTexture(t->bg);
}

void TitleRefresh(UiTitle *t)
{
    t->has_save = SceneSaveExists();
    /* Landing on a row that cannot be chosen is worse than not offering it,
       so the cursor is moved off LOAD rather than the row being hidden - a
       menu that changes length between visits is harder to learn. */
    if (!t->has_save && t->cursor == 1) t->cursor = 0;
}

void TitleUpdate(UiTitle *t, float dt) { t->clock += dt; }

void TitleMove(UiTitle *t, int dy)
{
    if (dy == 0) return;
    t->note = NULL;

    /* Wraps, and steps over LOAD when there is nothing to load, so holding a
       direction never parks on a dead row. */
    for (int guard = 0; guard < TITLE_ROWS; guard++) {
        t->cursor = (signed char)((t->cursor + dy + TITLE_ROWS) % TITLE_ROWS);
        if (t->cursor != 1 || t->has_save) return;
    }
}

TitleCmd TitleAccept(UiTitle *t)
{
    switch (t->cursor) {
    case 0: return TITLE_START;
    case 1:
        if (!t->has_save) { t->note = "No save to load."; return TITLE_NONE; }
        return TITLE_LOAD;
    case 2:
        /* Named rather than hidden. The row is a promise about where the
           setting will live, and an empty menu behind it would be a worse
           lie than saying so. */
        t->note = "Options are not built yet.";
        return TITLE_NONE;
    default: return TITLE_EXIT;
    }
}

/* Drawn twice, one pixel apart, because the 5x7 font has no bold and a title
   set in body text does not read as a title. */
static void DrawHeavy(const char *text, int y, Color shadow, Color face)
{
    const int x = (VSCREEN_W - UiTextWidth(text)) / 2;
    UiDrawText(text, x + 1, y + 1, shadow);
    UiDrawText(text, x, y, face);
}

void TitleDraw(const UiTitle *t)
{
    DrawTexture(t->bg, 0, 0, WHITE);

    /* The forge is a busy, high-contrast image and menu text sitting straight
       on it is unreadable wherever the fire is. The scrim is the cheapest fix
       that survives whatever real art replaces the placeholder. */
    DrawRectangle(0, 0, VSCREEN_W, VSCREEN_H, (Color){ 0, 0, 0, T_SCRIM });

    DrawHeavy(T_NAME, T_HEAD_Y, UI_SHADE, UI_TEXT);
    UiRule((VSCREEN_W - 170) / 2, T_HEAD_Y + 12, 170, UI_EDGE);
    UiDrawText(T_TAGLINE, (VSCREEN_W - UiTextWidth(T_TAGLINE)) / 2, T_SUB_Y,
               UI_DIM);

    for (int i = 0; i < TITLE_ROWS; i++) {
        const int y = T_MENU_Y + i * T_ROW_H;
        const int x = (VSCREEN_W - T_MENU_W) / 2;
        const bool on = (i == t->cursor);
        const bool dead = (i == 1 && !t->has_save);

        if (on) UiPanel(x, y, T_MENU_W, T_MENU_H, UI_SELECT, UI_EDGE);

        const Color tint = dead ? UI_SHADE : (on ? UI_TEXT : UI_DIM);
        UiDrawText(T_LABELS[i], (VSCREEN_W - UiTextWidth(T_LABELS[i])) / 2, y + 4,
                   tint);

        if (on) {
            const int frame = (int)(t->clock / T_BLINK);
            if ((frame & 1) == 0) UiDrawText(">", x + 8, y + 4, UI_TEXT);
        }
    }

    if (t->note != NULL) {
        UiDrawText(t->note, (VSCREEN_W - UiTextWidth(t->note)) / 2, T_NOTE_Y,
                   UI_DIM);
    }

    UiDrawText(T_FOOT, (VSCREEN_W - UiTextWidth(T_FOOT)) / 2, T_FOOT_Y, UI_DIM);
}
