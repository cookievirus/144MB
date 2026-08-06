#include "rarity.h"

/* ---- data -------------------------------------------------------------- */

/* One 8x8 ball, 2 bits per pixel, one row per uint16, pixel 0 in the high
   bits. 0 = transparent, 1 = rim, 2 = base, 3 = specular highlight.

     . . 1 1 1 1 . .
     . 1 3 3 2 2 1 .
     1 2 3 3 2 2 2 1
     1 2 2 2 2 2 2 1
     1 2 2 2 2 2 2 1
     1 2 2 2 2 2 1 1
     . 1 2 2 2 1 1 .
     . . 1 1 1 1 . .

   Three visible shades is the ceiling at this size; a 4 bpp mask would cost
   32 bytes for fifteen shades that nobody can resolve in an 8-pixel circle. */
static const unsigned short RARITY_MASK[RARITY_BALL_H] = {
    0x0550, 0x1FA4, 0x6FA9, 0x6AA9,
    0x6AA9, 0x6AA5, 0x1A94, 0x0550
};

/* Base hue only; rim and highlight are derived. 0x00RRGGBB.

   Junk is rust and Common is clean steel rather than two greys. Grey-on-grey
   is the conventional pairing and it does not survive an 8-pixel ball with a
   white highlight on it - the two read as the same object. */
static const unsigned int RARITY_BASE[RARITY_COUNT] = {
    0x7A6248,  /* Junk      - rust        */
    0xDCE0E8,  /* Common    - clean steel */
    0x40C048,  /* Uncommon  - green       */
    0x3080E8,  /* Rare      - blue        */
    0xA850E0,  /* Epic      - purple      */
    0xF09818,  /* Legendary - orange      */
    0xD83028,  /* Myth      - red         */
    0x483C58   /* Cursed    - void        */
};

/* Price multiplier in eighths: 8 == 1.0x. Cursed pays well, which is the
   whole hook - the shop books the profit and then lives with what the buyer
   does next. */
static const unsigned char RARITY_MUL[RARITY_COUNT] = {
    2, 8, 12, 20, 32, 56, 96, 24
};

/* Fixed stride, no offset table. "Legendary" plus its terminator is the
   longest entry. Packing these with a byte offset table would save 19 bytes
   and cost an indirection on every draw. */
static const char RARITY_LABEL[RARITY_COUNT][10] = {
    "Junk", "Common", "Uncommon", "Rare",
    "Epic", "Legendary", "Myth", "Cursed"
};

static Texture2D rarity_tex;   /* RARITY_COUNT * 8 wide, 8 tall */

/* ---- shading ----------------------------------------------------------- */

/* lvl 1 = rim at ~45%, 2 = base, 3 = highlight 80% of the way to white,
   4 = hue-preserving lift for text (see RarityTint). Integer throughout so
   this never reaches for libm. */
static Color RarityShade(unsigned int rgb, int lvl)
{
    int ch[3];

    ch[0] = (int)((rgb >> 16) & 0xFFu);
    ch[1] = (int)((rgb >> 8) & 0xFFu);
    ch[2] = (int)(rgb & 0xFFu);

    /* Scaling every channel by the same factor until the brightest one
       reaches 200 raises the value without touching the hue. Blending toward
       white instead would wash Cursed out into the same off-white as Common,
       which is the exact collision this palette exists to avoid. */
    int peak = ch[0];
    if (ch[1] > peak) peak = ch[1];
    if (ch[2] > peak) peak = ch[2];
    if (peak < 1) peak = 1;

    for (int i = 0; i < 3; i++) {
        if (lvl == 1)      ch[i] = (ch[i] * 116) >> 8;
        else if (lvl == 3) ch[i] = ch[i] + (((255 - ch[i]) * 205) >> 8);
        else if (lvl == 4) ch[i] = ch[i] * 200 / peak;
    }

    return (Color){ (unsigned char)ch[0], (unsigned char)ch[1],
                    (unsigned char)ch[2], 255 };
}

static Rarity RarityClamp(Rarity r)
{
    return (r < 0 || r >= RARITY_COUNT) ? RARITY_COMMON : r;
}

/* ---- lifecycle --------------------------------------------------------- */

void RarityLoad(void)
{
    const int w = RARITY_COUNT * RARITY_BALL_W;
    const int h = RARITY_BALL_H;

    Color *pixels = (Color *)MemAlloc((unsigned int)(w * h) * sizeof(Color));
    if (pixels == NULL) return;

    for (int r = 0; r < RARITY_COUNT; r++) {
        const Color lut[4] = {
            (Color){ 0, 0, 0, 0 },
            RarityShade(RARITY_BASE[r], 1),
            RarityShade(RARITY_BASE[r], 2),
            RarityShade(RARITY_BASE[r], 3)
        };

        for (int y = 0; y < RARITY_BALL_H; y++) {
            const unsigned short row = RARITY_MASK[y];
            for (int x = 0; x < RARITY_BALL_W; x++) {
                const int lvl = (row >> (14 - x * 2)) & 3;
                pixels[y * w + r * RARITY_BALL_W + x] = lut[lvl];
            }
        }
    }

    Image img = {
        .data = pixels, .width = w, .height = h,
        .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    rarity_tex = LoadTextureFromImage(img);
    SetTextureFilter(rarity_tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
}

void RarityUnload(void)
{
    UnloadTexture(rarity_tex);
}

/* ---- drawing ----------------------------------------------------------- */

void RarityBall(Rarity r, int x, int y)
{
    r = RarityClamp(r);

    const Rectangle src = {
        (float)((int)r * RARITY_BALL_W), 0.0f,
        (float)RARITY_BALL_W, (float)RARITY_BALL_H
    };
    DrawTextureRec(rarity_tex, src, (Vector2){ (float)x, (float)y }, WHITE);
}

Color RarityTint(Rarity r)
{
    r = RarityClamp(r);

    const unsigned int c = RARITY_BASE[r];

    /* Rec.601 luma in 8.8 fixed point. Anything below the threshold would
       disappear into UI_SHADE, so it is lifted instead. This is why Cursed
       needs no separate text-colour table.

       The margin matters: at a threshold of 72, Cursed sat three points below
       the line and any future tweak to its base would silently flip it to the
       unlifted branch. Myth, the next darkest tier, sits at 97. */
    const int lum = (int)((int)((c >> 16) & 0xFFu) * 77 +
                          (int)((c >> 8) & 0xFFu) * 151 +
                          (int)(c & 0xFFu) * 28) >> 8;

    return RarityShade(c, (lum < 80) ? 4 : 2);
}

const char *RarityName(Rarity r)
{
    return RARITY_LABEL[RarityClamp(r)];
}

int RarityScaleValue(int base_price, Rarity r)
{
    return (base_price * (int)RARITY_MUL[RarityClamp(r)]) >> 3;
}

/* ---- future ------------------------------------------------------------ */
/* If a "Cursed Legendary Greatsword" ever has to exist, lift CURSED out of
   this enum into a one-bit flag on ItemDef and drop RARITY_COUNT to 7. The
   ball would then draw the tier colour with the rim forced to black: roughly
   twenty extra bytes of code and no new data. Not worth doing until the drama
   engine actually asks for it. */
