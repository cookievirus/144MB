#include "vfx.h"

/* Macros prefixed V_, statics prefixed Vfx. Unity build: file scope is not
   scope, and this module's Blob and Noise are exactly the names a second
   effects module would want. */

/* ---- tuning ------------------------------------------------------------ */

#define V_EMBER_MIN_LIFE  0.75f
#define V_EMBER_LIFE_VAR  0.95f
#define V_EMBER_RISE_MIN  9.0f     /* px per second */
#define V_EMBER_RISE_VAR 14.0f
#define V_EMBER_DRIFT     5.0f     /* px per second, either way */
#define V_EMBER_WOBBLE    3.2f     /* px of sideways sway       */

/* Per kind: two flicker rates, the dimmest the light is allowed to get, the
   core alpha and the colour it adds.

   The rate pairs are deliberately not multiples of each other, so the sum
   does not come back round to where it started on any interval a player would
   sit through.

   The lamp floor was 0.74 in 1.9.1 and is 0.62 now. The peak alpha a lamp
   adds is around 40 of 255, so a floor that high moved the light by three
   parts in a hundred - a swing the tuning file could describe and the eye
   could not see. Cheap to measure and easy to get wrong: what matters is the
   change against the backdrop, not the change against the light's own
   maximum. A hearth is fast and deep because a forge is being fed air; a
   lamp is slow and shallow because an oil wick is not. Getting that wrong in
   either direction is what makes an effect read as "the game is doing
   something" rather than "the room is lit". */
typedef struct LightTune {
    float slow, fast;          /* Hz                             */
    float floor;               /* dimmest the light may go       */
    unsigned char alpha;       /* total added at the very centre */
    unsigned char layers;      /* concentric steps in the falloff */
    float swell;               /* how much the halo breathes, 0..1 */
    Color tint;
} LightTune;

static const LightTune V_TUNE[LIGHT_KIND_COUNT] = {
    /* HEARTH */ {  5.3f, 14.7f, 0.58f, 70, 8, 0.10f, { 255, 146,  40, 255 } },
    /* LAMP   */ {  1.9f,  4.3f, 0.62f, 44, 7, 0.14f, { 255, 182,  92, 255 } },
};

static const Color V_HOT  = { 255, 228, 160, 255 };   /* a new ember  */
static const Color V_COOL = { 198,  70,  24, 255 };   /* a dying one  */

/* ---- noise -------------------------------------------------------------

   An integer hash and a smoothed interpolation between its outputs. Not a
   sine: fire does not flicker sinusoidally, and sinf would drag libm's
   trigonometry in for one visual effect. */

static float VfxHash(unsigned int n)
{
    n ^= n << 13;
    n ^= n >> 17;
    n ^= n << 5;
    return (float)(n & 0xFFFFu) / 65535.0f;
}

static float VfxWave(float t, float rate, unsigned int seed)
{
    const float p = t * rate;
    const int i = (int)p;
    const float fr = p - (float)i;
    /* Smoothstep, so the value has no corners where the samples meet. */
    const float s = fr * fr * (3.0f - 2.0f * fr);
    const float a = VfxHash(seed + (unsigned int)i);
    const float b = VfxHash(seed + (unsigned int)i + 1u);
    return a + (b - a) * s;
}

static const LightTune *VfxTune(const LightDef *d)
{
    return &V_TUNE[(d->kind < LIGHT_KIND_COUNT) ? d->kind : LIGHT_HEARTH];
}

/* ---- lifecycle --------------------------------------------------------- */

static void VfxSpawn(VfxRoom *f, Ember *e, bool mid_life)
{
    const LightDef *d = &f->light[f->sparker];
    e->seed += 0x9E37u;

    const float rx = VfxHash(e->seed);
    const float rv = VfxHash(e->seed ^ 0x5A5Au);
    const float rl = VfxHash(e->seed ^ 0x1234u);
    const float rd = VfxHash(e->seed ^ 0x77F1u);

    e->x = (float)d->x - (float)d->bed_w * 0.5f + rx * (float)d->bed_w;
    e->y = (float)d->y + (float)d->bed_dy;
    e->vy = -(V_EMBER_RISE_MIN + rv * V_EMBER_RISE_VAR);
    e->vx = (rd - 0.5f) * 2.0f * V_EMBER_DRIFT;
    e->span = V_EMBER_MIN_LIFE + rl * V_EMBER_LIFE_VAR;
    e->life = e->span;

    /* On a cold start every ember would be born on the coals on the same
       frame and the first second would be one visible pulse. Scattering the
       initial ages up the column means the fire is already running when the
       room appears. */
    if (mid_life) {
        const float age = VfxHash(e->seed ^ 0xBEEFu) * e->span;
        e->life -= age;
        e->x += e->vx * age;
        e->y += e->vy * age;
    }
}

void VfxRoomStart(VfxRoom *f, const LightDef *lights, int count)
{
    if (count > SCENE_MAX_LIGHTS) count = SCENE_MAX_LIGHTS;

    f->light = (lights != NULL && count > 0) ? lights : NULL;
    f->count = (unsigned char)((f->light != NULL) ? count : 0);
    f->sparker = -1;
    f->clock = 0.0f;
    if (f->light == NULL) return;

    for (int i = 0; i < f->count; i++) {
        if (f->light[i].embers > 0) { f->sparker = (signed char)i; break; }
    }
    if (f->sparker < 0) return;

    for (int i = 0; i < VFX_EMBERS; i++) {
        f->ember[i].seed = 0x2545F491u + (unsigned int)i * 0x27D4EB2Du;
        VfxSpawn(f, &f->ember[i], true);
    }
}

void VfxRoomStop(VfxRoom *f)
{
    f->light = NULL;
    f->count = 0;
    f->sparker = -1;
}

bool VfxRoomIsLive(const VfxRoom *f) { return f->light != NULL && f->count > 0; }

float VfxLightGlow(const VfxRoom *f, int index)
{
    if (f->light == NULL || index < 0 || index >= (int)f->count) return 0.0f;

    const LightTune *t = VfxTune(&f->light[index]);

    /* Each light gets its own phase from its index, so three lamps in one
       room do not breathe in unison - which is the single thing that would
       give the whole effect away as one timer. */
    const unsigned int s1 = 0x1000u + (unsigned int)index * 0x3B9Au;
    const unsigned int s2 = 0x8000u + (unsigned int)index * 0x51EDu;

    const float slow = VfxWave(f->clock, t->slow, s1);
    const float fast = VfxWave(f->clock, t->fast, s2);
    const float g = t->floor + (1.0f - t->floor) * (slow * 0.7f + fast * 0.3f);
    return (g > 1.0f) ? 1.0f : g;
}

/* ---- update ------------------------------------------------------------ */

void VfxRoomUpdate(VfxRoom *f, float dt)
{
    if (f->light == NULL) return;

    f->clock += dt;
    if (f->sparker < 0) return;

    const int n = (f->light[f->sparker].embers < VFX_EMBERS)
                ? f->light[f->sparker].embers : VFX_EMBERS;

    for (int i = 0; i < n; i++) {
        Ember *e = &f->ember[i];
        e->life -= dt;
        /* Respawned the instant it dies rather than on a timer, so the
           population is constant and there is no emitter state to keep. */
        if (e->life <= 0.0f) { VfxSpawn(f, e, false); continue; }

        e->x += e->vx * dt;
        e->y += e->vy * dt;
    }
}

/* ---- drawing -----------------------------------------------------------

   Additive, because alpha over a dark backdrop greys it and over a bright one
   washes it out; a light adds to what is behind it and nothing else in the
   game does. */

/* Binary search rather than sqrtf. Nine iterations for anything a radius on
   this screen can produce, no libm, and obviously correct at a glance -
   which the bit-by-bit integer square root is not. */
static int VfxISqrt(int v)
{
    int lo = 0, hi = 256;
    while (lo < hi) {
        const int mid = (lo + hi + 1) / 2;
        if (mid * mid <= v) lo = mid; else hi = mid - 1;
    }
    return lo;
}

/* One flat-alpha ellipse. Rows are the true ellipse width, not the quadratic
   approximation 1.9 used: that made a lens, narrower at mid-height than a
   circle, which is part of why the lamps did not read as round. */
static void VfxBlob(int cx, int cy, int rx, int ry, int alpha, Color tint)
{
    if (rx <= 0 || ry <= 0 || alpha <= 0) return;

    for (int dy = -ry; dy <= ry; dy++) {
        const int w = (rx * VfxISqrt(ry * ry - dy * dy)) / ry;
        if (w <= 0) continue;
        DrawRectangle(cx - w, cy + dy, w * 2, 1,
                      (Color){ tint.r, tint.g, tint.b, (unsigned char)alpha });
    }
}

/* A soft round glow, built by stacking flat ellipses of decreasing radius and
   letting the additive blend do the falloff.

   1.9 shaded each row by its height alone and kept the alpha flat across the
   row's width, so the top and bottom faded and the left and right ends were
   cut off square. At a 4 px radius nobody could tell; at the radius a lamp
   halo wants, it reads as a bar, not a light.

   Doing it per pixel would be a DrawRectangle per pixel - about 500 a lamp -
   to compute a falloff the blend can accumulate for free. Stacking N flat
   ellipses gives a genuinely radial profile for N calls: at any distance the
   accumulated alpha is the sum of the layers that still cover it.

   Layer alphas are weighted (N, N-1, ... 1) rather than equal, which makes
   that sum quadratic in distance rather than linear - a bright core with a
   long thin skirt, instead of a flat disc with an edge. */
static void VfxGlow(int cx, int cy, int rx, int ry, int alpha, int layers,
                    Color tint)
{
    if (layers < 1) layers = 1;
    const int denom = layers * (layers + 1) / 2;

    for (int i = layers; i >= 1; i--) {
        const int a = (alpha * (layers - i + 1)) / denom;
        if (a <= 0) continue;
        VfxBlob(cx, cy, (rx * i) / layers, (ry * i) / layers, a, tint);
    }
}

static void VfxDrawEmbers(const VfxRoom *f, float glow)
{
    const LightDef *d = &f->light[f->sparker];
    const int n = (d->embers < VFX_EMBERS) ? d->embers : VFX_EMBERS;

    for (int i = 0; i < n; i++) {
        const Ember *e = &f->ember[i];
        if (e->life <= 0.0f) continue;

        /* 1 at birth, 0 at death. Drives colour and alpha together: an ember
           cools and dims at the same time, which is the whole read. */
        const float t = e->life / e->span;

        /* A slow sideways sway on top of the constant drift, seeded per
           ember, so fourteen of them do not rise in parallel. */
        const float sway = (VfxWave(f->clock, 2.4f, e->seed) - 0.5f)
                         * 2.0f * V_EMBER_WOBBLE * (1.0f - t);

        const int px = (int)(e->x + sway + 0.5f);
        const int py = (int)(e->y + 0.5f);

        /* `rise` is a fade envelope, not a clip. It was a clip for about ten
           minutes and the 30-second trace caught it: a slow, long-lived ember
           climbs 39 px on a 30 px ceiling, so it was still at a third of its
           brightness when it hit the line and simply stopped existing. A
           particle that blinks out mid-air is more noticeable than one that
           was never drawn. */
        const int climb = (int)d->y + (int)d->bed_dy - py;
        if (climb >= (int)d->rise) continue;      /* already at zero alpha */
        const float head = (climb <= 0) ? 1.0f
                         : 1.0f - (float)climb / (float)d->rise;

        const Color c = {
            (unsigned char)((float)V_COOL.r + ((float)V_HOT.r - (float)V_COOL.r) * t),
            (unsigned char)((float)V_COOL.g + ((float)V_HOT.g - (float)V_COOL.g) * t),
            (unsigned char)((float)V_COOL.b + ((float)V_HOT.b - (float)V_COOL.b) * t),
            (unsigned char)(210.0f * t * glow * head)
        };

        DrawRectangle(px, py, 1, 1, c);
        /* The youngest few get a second pixel, so the bed reads as hotter
           than the air above it without a second particle system. */
        if (t > 0.72f) DrawRectangle(px, py + 1, 1, 1, c);
    }
}

void VfxRoomDraw(const VfxRoom *f)
{
    if (f->light == NULL) return;

    BeginBlendMode(BLEND_ADDITIVE);

    for (int i = 0; i < (int)f->count; i++) {
        const LightDef *d = &f->light[i];
        const LightTune *t = VfxTune(d);
        const float glow = VfxLightGlow(f, i);

        /* The halo breathes in size as well as brightness. A light that only
           changes alpha reads as a lamp on a dimmer; one whose reach moves a
           pixel with it reads as something burning. */
        const float swell = 1.0f - t->swell + t->swell * glow;

        VfxGlow((int)d->x, (int)d->y,
                (int)((float)d->rx * swell), (int)((float)d->ry * swell),
                (int)((float)t->alpha * glow), (int)t->layers, t->tint);
    }

    if (f->sparker >= 0) VfxDrawEmbers(f, VfxLightGlow(f, f->sparker));

    EndBlendMode();
}
