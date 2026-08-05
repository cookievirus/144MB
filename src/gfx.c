#include "gfx.h"

#include <math.h>
#include <stddef.h>

Texture2D GfxLoadTexture(const EmbeddedImage *src)
{
    Texture2D tex = { 0 };

    int expanded = 0;
    unsigned char *indices = DecompressData(src->blob, (int)src->blob_size, &expanded);
    if (indices == NULL) return tex;

    if ((unsigned int)expanded != src->raw_size) {
        TraceLog(LOG_WARNING, "GFX: payload size mismatch (%d vs %u)",
                 expanded, src->raw_size);
        MemFree(indices);
        return tex;
    }

    const int count = (int)src->w * (int)src->h;
    Color *pixels = (Color *)MemAlloc((unsigned int)count * sizeof(Color));
    if (pixels == NULL) {
        MemFree(indices);
        return tex;
    }

    for (int i = 0; i < count; i++) {
        const unsigned int slot = indices[i];
        const unsigned char *rgb = src->pal + slot * 3u;
        pixels[i].r = rgb[0];
        pixels[i].g = rgb[1];
        pixels[i].b = rgb[2];
        pixels[i].a = (src->color_key && slot == 0u) ? 0 : 255;
    }
    MemFree(indices);

    /* Built by hand rather than via LoadImageFromMemory so no image decoder
       is linked in. UnloadImage() releases `pixels` once it is on the GPU. */
    Image img = {
        .data = pixels,
        .width = src->w,
        .height = src->h,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

Rectangle GfxPillarboxDst(void)
{
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();

    float scale = fminf(sw / (float)VSCREEN_W, sh / (float)VSCREEN_H);
#if GFX_INTEGER_SCALE
    if (scale >= 1.0f) scale = floorf(scale);
#endif

    const float w = (float)VSCREEN_W * scale;
    const float h = (float)VSCREEN_H * scale;

    return (Rectangle){ floorf((sw - w) * 0.5f), floorf((sh - h) * 0.5f), w, h };
}

Rectangle GfxRenderTextureSrc(void)
{
    return (Rectangle){ 0.0f, 0.0f, (float)VSCREEN_W, -(float)VSCREEN_H };
}
