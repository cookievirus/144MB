/* vfx.h - the lights. Rising embers over a forge, a breathing bloom over
   anything that burns.

   The point is not to draw fire. The backdrops draw fire, and they draw it
   better than anything this module could afford to. The point is that a
   painted flame is a photograph of a flame, and a room whose only moving
   thing is the player reads as a menu with scenery. Two cheap moving elements
   over art that is already right is a far better trade than an animated
   hearth would be: a six-frame flame loop at the measured 32x22 would be
   ~4 KB of asset for one room, and it would still loop.

   Zero asset bytes. Rectangles and integer arithmetic, like every other drawn
   thing in the game, and no libm - the flicker is value noise over a hash
   rather than a sine, which is also what stops it looking like a pulse.

   The generator is private to this module and deliberately not the one in
   qte.c. Sharing a seed would make the forge minigame's sequence depend on
   how many frames the player spent looking at the fire, and the QTE tests
   assert exact sequences under a seed. */
#ifndef VFX_H
#define VFX_H

#include "ui.h"

/* Enough that the stream never visibly gaps, few enough that it reads as a
   fire rather than a fountain. Fourteen was chosen by looking at it. */
#define VFX_EMBERS 14

/* 1.9.1: rooms have more than one light. The shop has three - a wall lantern
   and two hanging lamps - and the smithy has its hearth. Four is what fits
   the two rooms that exist with headroom for the Guild's brazier; raising it
   costs twelve bytes per scene per slot and nothing else. */
#define SCENE_MAX_LIGHTS 4

/* A hearth roars and throws sparks. A lamp sits there. Same bloom, different
   tuning, and only a hearth carries embers - which falls out of `embers`
   being zero rather than needing to be asked separately. */
typedef enum LightKind {
    LIGHT_HEARTH = 0,
    LIGHT_LAMP,
    LIGHT_KIND_COUNT
} LightKind;

/* Where a light is, in virtual-screen pixels. Measured off the source art
   rather than guessed - see the tables in scene.c - so a re-export at a
   different crop fails a test instead of silently moving the glow onto a
   wall. */
typedef struct LightDef {
    unsigned short x, y;    /* centre of the glow                        */
    unsigned char rx, ry;   /* glow radii                                */
    unsigned char kind;     /* LightKind                                 */
    unsigned char embers;   /* 0 for anything that does not throw sparks */
    unsigned char bed_w;    /* width of the coal bed embers are born on  */
    unsigned char bed_dy;   /* how far below the centre that bed sits    */
    unsigned char rise;     /* how far an ember climbs before it is gone */
} LightDef;                 /* 12 bytes */

typedef struct Ember {
    float x, y;
    float vx, vy;
    float life;             /* seconds left  */
    float span;             /* seconds it began with */
    unsigned int seed;      /* bumped on every respawn */
} Ember;

/* One ember pool per room, owned by the first light that asks for one. A
   second sparking fire in the same room would silently get no sparks, which
   is a limitation and not a bug: no room has two forges, and the alternative
   is an array of pools sized for a case that does not exist. */
typedef struct VfxRoom {
    Ember ember[VFX_EMBERS];
    const LightDef *light;
    unsigned char count;
    signed char sparker;    /* index owning the embers, -1 for none */
    float clock;
} VfxRoom;

/* lights may be NULL or count may be 0. Both mean a room with nothing burning
   in it, which costs one branch per frame and draws nothing. */
void VfxRoomStart(VfxRoom *f, const LightDef *lights, int count);
void VfxRoomStop(VfxRoom *f);

bool VfxRoomIsLive(const VfxRoom *f);

void VfxRoomUpdate(VfxRoom *f, float dt);
void VfxRoomDraw(const VfxRoom *f);

/* 0.0 dim .. 1.0 bright for one light this frame. Exposed so a test can
   assert it stays in range and actually moves, which is the only part of a
   look that can be asserted. */
float VfxLightGlow(const VfxRoom *f, int index);

#endif /* VFX_H */
