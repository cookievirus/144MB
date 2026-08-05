/* gfx.h - virtual screen, pillarbox presentation, embedded image decoding. */
#ifndef GFX_H
#define GFX_H

#include "raylib.h"

/* Internal resolution. Everything is authored and drawn at this size; the
   backbuffer is upscaled once at present time. */
#define VSCREEN_W 320
#define VSCREEN_H 240

/* 1 = snap the upscale to whole multiples so every source pixel stays square.
   Costs slightly larger letterbox bars; buys a clean DOS/NES look. */
#define GFX_INTEGER_SCALE 1

/* A palettised image baked into .rodata by tools/png2c.py.
   Payload is a raw DEFLATE stream of `w * h` 8-bit palette indices, which
   raylib's DecompressData() (sinfl) expands at load time. Storing indices
   rather than PNG lets raylib's image decoders be stripped in config.h. */
typedef struct EmbeddedImage {
    unsigned short w, h;
    unsigned short colors;              /* palette entries, max 256          */
    unsigned short color_key;           /* 1 = index 0 is fully transparent  */
    const unsigned char *pal;           /* colors * 3 bytes, RGB             */
    const unsigned char *blob;          /* raw DEFLATE stream                */
    unsigned int blob_size;
    unsigned int raw_size;              /* expected w * h, used as a check   */
} EmbeddedImage;

/* Decodes into a GPU texture with point filtering. Returns a texture with
   id == 0 if the payload is corrupt. */
Texture2D GfxLoadTexture(const EmbeddedImage *src);

/* Destination rectangle that centres the virtual screen in the window. */
Rectangle GfxPillarboxDst(void);

/* Source rectangle for a RenderTexture2D (negative height flips the y axis). */
Rectangle GfxRenderTextureSrc(void);

#endif /* GFX_H */
