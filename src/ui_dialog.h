/* ui_dialog.h - translucent dialogue balloon with portrait and name plate. */
#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include "ui_font.h"

/* Frame index into the portrait strip. Order follows HERO-BEST-MOOD-01
   read left to right, top to bottom. */
typedef enum HeroMood {
    MOOD_FURIOUS = 0, MOOD_ANGRY,   MOOD_SURPRISED, MOOD_SMILE,  MOOD_STERN,
    MOOD_SHOCKED,     MOOD_NERVOUS, MOOD_SULKY,     MOOD_HUFF,   MOOD_TIRED,
    MOOD_SMIRK,       MOOD_GLUM,    MOOD_ENRAGED,   MOOD_SHOUT,  MOOD_YELL,
    MOOD_CRYING,      MOOD_SIGH,    MOOD_CALM,      MOOD_WINK,   MOOD_LAUGH,
    MOOD_COUNT
} HeroMood;

/* How text arrives. Word mode snaps to the next space, which keeps whole
   words intact instead of letting them grow letter by letter. */
typedef enum DialogReveal {
    REVEAL_WORD = 0,
    REVEAL_CHAR
} DialogReveal;

typedef enum DialogPhase {
    DIALOG_HIDDEN = 0,
    DIALOG_REVEALING,
    DIALOG_WAITING      /* fully revealed, waiting for the player */
} DialogPhase;

/* Which strip a line's portrait comes from.

   One combined 40-frame strip would have been one texture and one bind, but
   it would also have been one 16-colour palette shared between a cream-and-
   brown smith and a green-hooded trader, and both faces would have paid for
   it. Two strips of 16 beat one strip of 16; a single strip of 32 would cost
   more payload than the second bind saves. */
typedef enum PortraitSet {
    PORTRAIT_HERO = 0,
    PORTRAIT_MERCHANT,
    PORTRAIT_SET_COUNT
} PortraitSet;

typedef struct DialogLine {
    const char *speaker;      /* NULL hides the name plate */
    const char *text;         /* '\n' breaks a line; wrapping is not automatic */
    unsigned char mood;       /* HeroMood; ignored when portraits are off */
    unsigned char set;        /* PortraitSet; lands in existing padding */
} DialogLine;

typedef struct UiDialog {
    Texture2D portraits[PORTRAIT_SET_COUNT];
    DialogPhase phase;
    DialogReveal reveal;
    const DialogLine *line;
    int shown;                /* characters currently visible */
    int total;                /* characters in the current line */
    float timer;              /* accumulates toward the next reveal step */
    float blink;              /* continue-caret blink accumulator */
} UiDialog;

void UiDialogInit(UiDialog *d);
void UiDialogUnload(UiDialog *d);

void UiDialogShow(UiDialog *d, const DialogLine *line);
void UiDialogHide(UiDialog *d);

void UiDialogUpdate(UiDialog *d, float dt);

/* Player pressed the advance key. Skips to the end of the line if it is still
   revealing; otherwise closes the balloon. Returns true if the line finished
   and the caller should move on. */
bool UiDialogAdvance(UiDialog *d);

void UiDialogDraw(const UiDialog *d);

#endif /* UI_DIALOG_H */
