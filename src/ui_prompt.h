/* ui_prompt.h - centred modal popup with a row of choices.

   Written generically rather than as a bespoke quit box because the same
   shape is wanted for every confirmation the game will need later: selling a
   blade, travelling, abandoning a commission. A one-off quit dialog would be
   the same code with the labels welded in. */
#ifndef UI_PROMPT_H
#define UI_PROMPT_H

#include "ui.h"

#define PROMPT_MAX_OPTIONS 4

/* Returned by UiPromptAccept when nothing was chosen. */
#define PROMPT_NONE (-1)

typedef struct UiPrompt {
    const char *title;
    const char *note;                             /* dim line under the row */
    const char *options[PROMPT_MAX_OPTIONS];
    unsigned char count;
    signed char cursor;
    bool open;
} UiPrompt;

void UiPromptOpen(UiPrompt *p, const char *title,
                  const char *const *options, int count, int initial);
void UiPromptClose(UiPrompt *p);
bool UiPromptIsOpen(const UiPrompt *p);

void UiPromptMove(UiPrompt *p, int dx);

/* Returns the chosen index and closes, or PROMPT_NONE if not open. */
int UiPromptAccept(UiPrompt *p);

void UiPromptDraw(const UiPrompt *p);

#endif /* UI_PROMPT_H */
