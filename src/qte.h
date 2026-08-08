/* qte.h - the sequential quick-time event that is the act of forging.

   The list screen decides what is being made and the ore is already gone by
   the time this opens; everything here decides only how well it comes out.
   That split is why inventory.c has InvSpendMaterials and InvGrantItem rather
   than the single InvForge it had in 1.5: a ruined heat is a spend with no
   grant, and if the two were still welded together there would be no way to
   express one.

   The sequence always terminates. A wrong key and an expired window are the
   same event - a miss that advances the step - so the minigame cannot be
   stalled by not playing it, and there is no need for an abandon key. ESC is
   ignored while it is up, which is the one place in the game where that is
   true: the metal is in the fire, and a reflexive back-key that threw the ore
   away would be the exact thing the ESC ladder exists to prevent.

   No score, no combo counter, no numbers. The player is told which key is
   live, how much of the sequence is left, and how much time this step has -
   and nothing else, because everything else is bookkeeping the smith would
   not be looking at. */
#ifndef QTE_H
#define QTE_H

#include "ui.h"
#include "inventory.h"

/* The five inputs, in the order the reference art lays them out. Values are
   an index into the glyph switch in qte.c, not raylib key codes: the scene
   translates its own dx/dy and accept key into these, so the minigame never
   learns what a keyboard is and the headless tests can drive it directly. */
typedef enum QteKey {
    QK_RIGHT = 0,
    QK_UP,
    QK_DOWN,
    QK_SPACE,
    QK_LEFT,
    QK_KEY_COUNT
} QteKey;

#define QTE_MAX_STEPS 8

typedef enum QteGrade {
    QTE_FINE = 0,   /* flawless: the recipe's better output, where it has one */
    QTE_PLAIN,      /* within tolerance: the recipe's normal output           */
    QTE_RUINED      /* too many misses: the ore is gone and nothing was made  */
} QteGrade;

typedef struct UiQte {
    unsigned char step[QTE_MAX_STEPS];
    unsigned char steps;
    unsigned char at;
    unsigned char misses;
    unsigned short missed;      /* bit per step, for the dimmed-red readout */

    signed char recipe;
    float timer;                /* seconds left on the current step */
    float window;               /* seconds this step was given      */
    float flash;                /* hit/miss feedback, counts down   */
    signed char flash_kind;     /* +1 hit, -1 miss                  */

    bool open;
    bool done;                  /* sequence over, showing the verdict */
    float done_time;
    unsigned char grade;        /* QteGrade, valid once done         */
} UiQte;

/* Deterministic and self-contained: one xorshift word rather than rand() or
   raylib's GetRandomValue, so the tests can replay an exact sequence and
   neither libc's generator nor a stub for it is linked in. */
void QteSeed(unsigned int seed);

void QteInit(UiQte *q);
bool QteIsOpen(const UiQte *q);

/* Materials must already have been spent. Builds a sequence whose length
   comes from the tier of the thing being made. */
void QteBegin(UiQte *q, int recipe);

void QteKeyPress(UiQte *q, QteKey key);

void QteUpdate(UiQte *q, float dt);

/* True once the verdict has been on screen long enough for the scene to
   collect it. Reading the grade does not close the widget; QteClose does. */
bool QteFinished(const UiQte *q);
QteGrade QteResult(const UiQte *q);
void QteClose(UiQte *q);

/* Which ItemId this grade produces, or -1 for a ruined heat. */
int QteOutput(int recipe, QteGrade grade);

void QteDraw(const UiQte *q);

#endif /* QTE_H */
