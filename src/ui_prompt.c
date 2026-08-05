#include "ui_prompt.h"

#include "ui_font.h"

#define PANEL_W   152
#define PANEL_H    54
#define PANEL_Y    94
#define BTN_H      14
#define BTN_PAD     5
#define BTN_GAP     4
#define TITLE_Y   (PANEL_Y + 7)
#define BTN_Y     (PANEL_Y + 21)
#define NOTE_Y    (PANEL_Y + 39)

#define SCREEN_MID (VSCREEN_W / 2)

static int BtnWidth(const char *label)
{
    return UiTextWidth(label) + BTN_PAD * 2;
}

static int RowWidth(const UiPrompt *p)
{
    int w = 0;
    for (int i = 0; i < p->count; i++) {
        w += BtnWidth(p->options[i]);
        if (i + 1 < p->count) w += BTN_GAP;
    }
    return w;
}

void UiPromptOpen(UiPrompt *p, const char *title,
                  const char *const *options, int count, int initial)
{
    if (count > PROMPT_MAX_OPTIONS) count = PROMPT_MAX_OPTIONS;
    p->title = title;
    p->note = NULL;
    p->count = (unsigned char)count;
    p->cursor = (signed char)((initial < 0 || initial >= count) ? 0 : initial);
    p->open = true;
    for (int i = 0; i < count; i++) p->options[i] = options[i];
}

void UiPromptClose(UiPrompt *p) { p->open = false; }
bool UiPromptIsOpen(const UiPrompt *p) { return p->open; }

void UiPromptMove(UiPrompt *p, int dx)
{
    if (!p->open || dx == 0 || p->count == 0) return;
    p->cursor = (signed char)(((p->cursor + dx) % p->count + p->count) % p->count);
}

int UiPromptAccept(UiPrompt *p)
{
    if (!p->open) return PROMPT_NONE;
    const int choice = p->cursor;
    p->open = false;
    return choice;
}

void UiPromptDraw(const UiPrompt *p)
{
    if (!p->open) return;

    /* A wash over the whole frame, which the command menu deliberately does
       not do. Modal and non-modal should not look the same. */
    DrawRectangle(0, 0, VSCREEN_W, VSCREEN_H, (Color){ 0, 0, 0, 130 });

    const int x = SCREEN_MID - PANEL_W / 2;
    UiPanel(x, PANEL_Y, PANEL_W, PANEL_H, UI_FILL, UI_EDGE);

    UiDrawText(p->title, SCREEN_MID - UiTextWidth(p->title) / 2, TITLE_Y, UI_TEXT);

    int bx = SCREEN_MID - RowWidth(p) / 2;
    for (int i = 0; i < p->count; i++) {
        const int bw = BtnWidth(p->options[i]);
        const bool on = (i == p->cursor);
        UiPanel(bx, BTN_Y, bw, BTN_H, on ? UI_SELECT : UI_SHADE, UI_EDGE);
        UiDrawText(p->options[i], bx + BTN_PAD, BTN_Y + 3, on ? UI_TEXT : UI_DIM);
        bx += bw + BTN_GAP;
    }

    if (p->note != NULL) {
        UiDrawText(p->note, SCREEN_MID - UiTextWidth(p->note) / 2, NOTE_Y, UI_DIM);
    }
}
