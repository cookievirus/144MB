#include "ui_dialog.h"
#include "ui.h"

#include <string.h>

#include "../assets/portraits.h"

/* Layout, in virtual-screen pixels. The balloon hugs the bottom of the frame
   and the name plate overhangs its top-left corner, as in the concept. */
#define BOX_X      8
#define BOX_Y    166
#define BOX_W    304
#define BOX_H     66

#define NAME_X    12
#define NAME_Y   152
#define NAME_H    13
#define NAME_PAD   6

#define PORT_SIZE 48
#define PORT_X    14
#define PORT_Y   174

#define TEXT_X    70
#define TEXT_Y   176
#define TEXT_MAX_COLS 39   /* (BOX_X + BOX_W - 6 - TEXT_X) / FONT_CELL_W */

#define CARET_X  160
#define CARET_Y  222
#define CARET_BLINK 0.45f

/* Reveal speed. Word mode ticks slower per step because each step delivers
   several characters. */
#define CHAR_STEP 0.030f
#define WORD_STEP 0.105f

static int NextWordEnd(const char *s, int from)
{
    int i = from;
    while (s[i] == ' ' || s[i] == '\n') i++;
    while (s[i] != '\0' && s[i] != ' ' && s[i] != '\n') i++;
    return i;
}

void UiDialogInit(UiDialog *d)
{
    d->portraits = GfxLoadTexture(&portraits);
    d->phase = DIALOG_HIDDEN;
    d->reveal = REVEAL_WORD;
    d->line = NULL;
    d->shown = 0;
    d->total = 0;
    d->timer = 0.0f;
    d->blink = 0.0f;
}

void UiDialogUnload(UiDialog *d)
{
    UnloadTexture(d->portraits);
}

void UiDialogShow(UiDialog *d, const DialogLine *line)
{
    d->line = line;
    d->total = (int)strlen(line->text);
    d->shown = 0;
    d->timer = 0.0f;
    d->blink = 0.0f;
    d->phase = (d->total > 0) ? DIALOG_REVEALING : DIALOG_WAITING;
    TraceLog(LOG_INFO, "DIALOG: %s (%d chars, mood=%d)", line->speaker, d->total, line->mood);
}

void UiDialogHide(UiDialog *d)
{
    d->phase = DIALOG_HIDDEN;
    d->line = NULL;
}

void UiDialogUpdate(UiDialog *d, float dt)
{
    if (d->phase == DIALOG_HIDDEN) return;

    d->blink += dt;

    if (d->phase != DIALOG_REVEALING) return;

    const float step = (d->reveal == REVEAL_WORD) ? WORD_STEP : CHAR_STEP;
    d->timer += dt;

    while (d->timer >= step) {
        d->timer -= step;
        d->shown = (d->reveal == REVEAL_WORD)
                       ? NextWordEnd(d->line->text, d->shown)
                       : d->shown + 1;
        if (d->shown >= d->total) {
            d->shown = d->total;
            d->phase = DIALOG_WAITING;
            d->blink = 0.0f;
            break;
        }
    }
}

bool UiDialogAdvance(UiDialog *d)
{
    if (d->phase == DIALOG_HIDDEN) return false;

    if (d->phase == DIALOG_REVEALING) {
        /* First press completes the line rather than skipping it, which is
           what a player who is simply reading fast expects. */
        d->shown = d->total;
        d->phase = DIALOG_WAITING;
        d->blink = 0.0f;
        return false;
    }

    UiDialogHide(d);
    return true;
}

void UiDialogDraw(const UiDialog *d)
{
    if (d->phase == DIALOG_HIDDEN || d->line == NULL) return;

    UiPanel(BOX_X, BOX_Y, BOX_W, BOX_H, UI_FILL, UI_EDGE);

    if (d->line->speaker != NULL) {
        const int w = UiTextWidth(d->line->speaker) + NAME_PAD * 2;
        UiPanel(NAME_X, NAME_Y, w, NAME_H, UI_PLATE, UI_EDGE);
        UiDrawText(d->line->speaker, NAME_X + NAME_PAD, NAME_Y + 3, UI_TEXT);
    }

    if (d->line->mood < MOOD_COUNT) {
        DrawRectangle(PORT_X - 1, PORT_Y - 1, PORT_SIZE + 2, PORT_SIZE + 2, UI_SHADE);
        const Rectangle src = {
            0.0f, (float)(d->line->mood * PORT_SIZE),
            (float)PORT_SIZE, (float)PORT_SIZE
        };
        DrawTextureRec(d->portraits, src, (Vector2){ PORT_X, PORT_Y }, WHITE);
    }

    UiDrawTextN(d->line->text, d->shown, TEXT_X, TEXT_Y, UI_TEXT);

    /* Continue caret: a blinking chevron, drawn as rows so it stays on the
       pixel grid instead of being rasterised as a triangle. */
    if (d->phase == DIALOG_WAITING) {
        const int frame = (int)(d->blink / CARET_BLINK);
        if ((frame & 1) == 0) {
            for (int i = 0; i < 4; i++) {
                DrawRectangle(CARET_X - 3 + i, CARET_Y + i, 7 - i * 2, 1, UI_TEXT);
            }
        }
    }
}
