/* ui_prompt.h - centred modal popup with a row or a column of choices.

   Written generically rather than as a bespoke quit box because the same
   shape is wanted for every confirmation the game will need later: selling a
   blade, travelling, abandoning a commission. A one-off quit dialog would be
   the same code with the labels welded in.

   1.8 made the box size itself. Until then PANEL_W was 152 and every caller
   was trusted to keep its title, its note and its buttons inside that - which
   held until 1.7 added "The forge goes cold until morning." at 204 px and it
   drew straight out through both walls. There was even a test for it, and the
   test enumerated the strings it knew about, so the one nobody remembered to
   add was the one that overflowed. A box that measures its own contents
   cannot have that bug; the test that replaces it asserts the invariant
   rather than a list. */
#ifndef UI_PROMPT_H
#define UI_PROMPT_H

#include "ui.h"

#define PROMPT_MAX_OPTIONS 4

/* Returned by UiPromptAccept when nothing was chosen. */
#define PROMPT_NONE (-1)

/* Buttons side by side, or stacked. A column is for a list of unlike actions
   the player reads one at a time - SAVE, CANCEL, QUIT - where a row invites
   them to be scanned as a spectrum. A row is for a question with two answers. */
typedef enum PromptLayout {
    PROMPT_ROW = 0,
    PROMPT_COLUMN
} PromptLayout;

/* Geometry. The box grows to its contents and stops here; a string that does
   not fit at this width does not fit on the screen. */
#define PROMPT_PAD    8
#define PROMPT_MIN_W  152
#define PROMPT_MAX_W  (VSCREEN_W - 16)
#define PROMPT_MAX_CHARS ((PROMPT_MAX_W - PROMPT_PAD * 2) / FONT_CELL_W)

/* Same bargain as UI_HINT_FITS: the budget is a constant, the check is at the
   definition, and a string one word too long is a build error naming itself. */
#define UI_PROMPT_FITS(s) \
    _Static_assert(sizeof(s) - 1 <= PROMPT_MAX_CHARS, \
                   "prompt line is wider than the screen: " s)

typedef struct UiPrompt {
    const char *title;
    const char *note;                             /* dim line under the row */
    const char *options[PROMPT_MAX_OPTIONS];
    unsigned char count;
    unsigned char layout;                         /* PromptLayout */
    signed char cursor;
    bool open;
} UiPrompt;

void UiPromptOpen(UiPrompt *p, const char *title,
                  const char *const *options, int count, int initial);

/* Same, stacked. */
void UiPromptOpenColumn(UiPrompt *p, const char *title,
                        const char *const *options, int count, int initial);

void UiPromptClose(UiPrompt *p);
bool UiPromptIsOpen(const UiPrompt *p);

/* The widget takes both axes and uses the one its layout runs along, so no
   caller has to know which layout is on screen to steer it. */
void UiPromptMove(UiPrompt *p, int dx, int dy);

/* Returns the chosen index and closes, or PROMPT_NONE if not open. */
int UiPromptAccept(UiPrompt *p);

/* The measured box, so a test can assert that what is drawn is inside it. */
int UiPromptWidth(const UiPrompt *p);
int UiPromptHeight(const UiPrompt *p);

void UiPromptDraw(const UiPrompt *p);

#endif /* UI_PROMPT_H */
