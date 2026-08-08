#include "ui_prompt.h"

#include "ui_font.h"

/* Vertical rhythm, top to bottom: pad, title, gap, buttons, gap, note, pad. */
#define P_EDGE     6            /* top and bottom inset                     */
#define P_LINE     7            /* one line of glyphs                       */
#define P_GAP      5
#define P_BTN_H   14
#define P_BTN_GAP  4
#define P_BTN_PAD  5

static int PromptBtnW(const char *label)
{
    return UiTextWidth(label) + P_BTN_PAD * 2;
}

/* In a column every button is as wide as the widest, so the stack reads as
   one control rather than three ragged ones. In a row they are their own
   width and the row is their sum. */
static int PromptWidestBtn(const UiPrompt *p)
{
    int w = 0;
    for (int i = 0; i < p->count; i++) {
        const int bw = PromptBtnW(p->options[i]);
        if (bw > w) w = bw;
    }
    return w;
}

static int PromptBtnBlockW(const UiPrompt *p)
{
    if (p->layout == PROMPT_COLUMN) return PromptWidestBtn(p);

    int w = 0;
    for (int i = 0; i < p->count; i++) {
        w += PromptBtnW(p->options[i]);
        if (i + 1 < p->count) w += P_BTN_GAP;
    }
    return w;
}

static int PromptBtnBlockH(const UiPrompt *p)
{
    if (p->layout != PROMPT_COLUMN) return P_BTN_H;
    return p->count * P_BTN_H + (p->count - 1) * P_BTN_GAP;
}

/* The widest thing in the box decides the box. Clamped at both ends: a
   minimum so a two-word question does not come out as a stamp, and a maximum
   because past it the box is wider than the screen and the answer is a
   shorter string, not a bigger panel. */
int UiPromptWidth(const UiPrompt *p)
{
    int w = PromptBtnBlockW(p);

    const int tw = (p->title != NULL) ? UiTextWidth(p->title) : 0;
    if (tw > w) w = tw;

    const int nw = (p->note != NULL) ? UiTextWidth(p->note) : 0;
    if (nw > w) w = nw;

    w += PROMPT_PAD * 2;
    if (w < PROMPT_MIN_W) w = PROMPT_MIN_W;
    if (w > PROMPT_MAX_W) w = PROMPT_MAX_W;
    return w;
}

int UiPromptHeight(const UiPrompt *p)
{
    int h = P_EDGE + P_LINE + P_GAP + PromptBtnBlockH(p) + P_EDGE;
    if (p->note != NULL) h += P_GAP + P_LINE;
    return h;
}

static void PromptInit(UiPrompt *p, const char *title,
                       const char *const *options, int count, int initial,
                       PromptLayout layout)
{
    if (count > PROMPT_MAX_OPTIONS) count = PROMPT_MAX_OPTIONS;
    p->title = title;
    p->note = NULL;
    p->count = (unsigned char)count;
    p->layout = (unsigned char)layout;
    p->cursor = (signed char)((initial < 0 || initial >= count) ? 0 : initial);
    p->open = true;
    for (int i = 0; i < count; i++) p->options[i] = options[i];
}

void UiPromptOpen(UiPrompt *p, const char *title,
                  const char *const *options, int count, int initial)
{
    PromptInit(p, title, options, count, initial, PROMPT_ROW);
}

void UiPromptOpenColumn(UiPrompt *p, const char *title,
                        const char *const *options, int count, int initial)
{
    PromptInit(p, title, options, count, initial, PROMPT_COLUMN);
}

void UiPromptClose(UiPrompt *p) { p->open = false; }
bool UiPromptIsOpen(const UiPrompt *p) { return p->open; }

void UiPromptMove(UiPrompt *p, int dx, int dy)
{
    if (!p->open || p->count == 0) return;

    /* Only the axis the buttons run along steers. Accepting the other one as
       well would mean a player nudging up in a two-button question silently
       changes their answer. */
    const int step = (p->layout == PROMPT_COLUMN) ? dy : dx;
    if (step == 0) return;

    p->cursor = (signed char)(((p->cursor + step) % p->count + p->count) % p->count);
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

    const int w = UiPromptWidth(p);
    const int h = UiPromptHeight(p);
    const int x = (VSCREEN_W - w) / 2;
    const int y = (VSCREEN_H - h) / 2;
    const int mid = x + w / 2;

    UiPanel(x, y, w, h, UI_FILL, UI_EDGE);

    int cy = y + P_EDGE;
    if (p->title != NULL) {
        UiDrawText(p->title, mid - UiTextWidth(p->title) / 2, cy, UI_TEXT);
    }
    cy += P_LINE + P_GAP;

    if (p->layout == PROMPT_COLUMN) {
        const int bw = PromptWidestBtn(p);
        for (int i = 0; i < p->count; i++) {
            const bool on = (i == p->cursor);
            const int by = cy + i * (P_BTN_H + P_BTN_GAP);
            UiPanel(mid - bw / 2, by, bw, P_BTN_H, on ? UI_SELECT : UI_SHADE,
                    UI_EDGE);
            /* Centred inside the shared width rather than left-aligned at the
               pad, or a short label sits off to one side of its own button. */
            UiDrawText(p->options[i],
                       mid - UiTextWidth(p->options[i]) / 2, by + 3,
                       on ? UI_TEXT : UI_DIM);
        }
    } else {
        int bx = mid - PromptBtnBlockW(p) / 2;
        for (int i = 0; i < p->count; i++) {
            const int bw = PromptBtnW(p->options[i]);
            const bool on = (i == p->cursor);
            UiPanel(bx, cy, bw, P_BTN_H, on ? UI_SELECT : UI_SHADE, UI_EDGE);
            UiDrawText(p->options[i], bx + P_BTN_PAD, cy + 3,
                       on ? UI_TEXT : UI_DIM);
            bx += bw + P_BTN_GAP;
        }
    }

    if (p->note != NULL) {
        cy += PromptBtnBlockH(p) + P_GAP;
        UiDrawText(p->note, mid - UiTextWidth(p->note) / 2, cy, UI_DIM);
    }
}
