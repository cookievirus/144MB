/* rarity.h - quality tier for every item, gear piece and fitted tool.

   Nothing here is a sprite. One 8x8 shape mask and one base colour per tier
   are baked into a single 64x8 strip at boot, which is why retuning the whole
   palette is a two-constant edit rather than an art pass.

   Data cost: 16 B mask + 32 B colours + 8 B multipliers + 80 B names = 136 B.
   Code cost: ~900 B measured with nm --size-sort on an -O2 build. Embedding
   eight finished sprites instead would be ~600 B cheaper and would need a
   regenerated asset header every time a colour moves. */
#ifndef RARITY_H
#define RARITY_H

#include "gfx.h"

/* A linear power ladder. CURSED sits at the top index because it is the most
   valuable, not the strongest - see the note at the foot of rarity.c. */
typedef enum Rarity {
    RARITY_JUNK = 0,
    RARITY_COMMON,
    RARITY_UNCOMMON,
    RARITY_RARE,
    RARITY_EPIC,
    RARITY_LEGENDARY,
    RARITY_MYTH,
    RARITY_CURSED,
    RARITY_COUNT
} Rarity;

#define RARITY_BALL_W 8
#define RARITY_BALL_H 8

/* Builds the ball strip. Must be called after InitWindow(). */
void RarityLoad(void);
void RarityUnload(void);

/* Draws the 8x8 ball with its top-left corner at (x, y). */
void RarityBall(Rarity r, int x, int y);

/* Tint for names, plates and rules. Near-black tiers are lifted so they stay
   legible against UI_SHADE. */
Color RarityTint(Rarity r);

const char *RarityName(Rarity r);

/* Fixed-point price scaling, eighths. No floats, no libm. Unused until the
   economy lands in Week 4; drop it if the Week 7 audit wants the 36 bytes. */
int RarityScaleValue(int base_price, Rarity r);

#endif /* RARITY_H */
