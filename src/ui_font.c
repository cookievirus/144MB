#include "ui_font.h"

#include <stddef.h>

#define FONT_GLYPH_COUNT (FONT_LAST_CHAR - FONT_FIRST_CHAR + 1)
#define FONT_LINE_H (FONT_CELL_H + 2)

/* All glyphs live side by side in one texture, so drawing a string is one
   DrawTextureRec per character instead of up to 35 one-pixel rectangles. */
static Texture2D font_tex;

void UiFontLoad(void)
{
    const int w = FONT_GLYPH_COUNT * FONT_CELL_W;
    const int h = FONT_CELL_H;

    Color *pixels = (Color *)MemAlloc((unsigned int)(w * h) * sizeof(Color));
    if (pixels == NULL) return;

    for (int i = 0; i < w * h; i++) pixels[i] = (Color){ 255, 255, 255, 0 };

    for (int g = 0; g < FONT_GLYPH_COUNT; g++) {
        const unsigned char *rows = font5x7_bits + g * FONT_GLYPH_H;
        for (int row = 0; row < FONT_GLYPH_H; row++) {
            for (int col = 0; col < FONT_GLYPH_W; col++) {
                if (rows[row] & (0x80u >> col)) {
                    pixels[row * w + g * FONT_CELL_W + col] =
                        (Color){ 255, 255, 255, 255 };
                }
            }
        }
    }

    Image img = {
        .data = pixels, .width = w, .height = h,
        .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    font_tex = LoadTextureFromImage(img);
    SetTextureFilter(font_tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    TraceLog(LOG_INFO, "FONT: atlas %dx%d %d glyphs", w, h, FONT_GLYPH_COUNT);
}

void UiFontUnload(void)
{
    UnloadTexture(font_tex);
}

int UiDrawTextN(const char *text, int count, int x, int y, Color tint)
{
    int pen_x = x;
    int pen_y = y;
    int drawn = 0;

    for (const char *p = text; *p != '\0'; p++) {
        if (count >= 0 && drawn >= count) break;
        drawn++;

        if (*p == '\n') {
            pen_x = x;
            pen_y += FONT_LINE_H;
            continue;
        }

        const unsigned char c = (unsigned char)*p;
        if (c >= FONT_FIRST_CHAR && c <= FONT_LAST_CHAR && c != ' ') {
            const Rectangle src = {
                (float)((c - FONT_FIRST_CHAR) * FONT_CELL_W), 0.0f,
                (float)FONT_CELL_W, (float)FONT_CELL_H
            };
            DrawTextureRec(font_tex, src, (Vector2){ (float)pen_x, (float)pen_y }, tint);
        }
        pen_x += FONT_CELL_W;
    }

    return pen_y + FONT_LINE_H;
}

int UiDrawText(const char *text, int x, int y, Color tint)
{
    return UiDrawTextN(text, -1, x, y, tint);
}

int UiTextWidth(const char *text)
{
    int best = 0;
    int run = 0;
    for (const char *p = text; *p != '\0'; p++) {
        if (*p == '\n') { run = 0; continue; }
        run += FONT_CELL_W;
        if (run > best) best = run;
    }
    return best;
}
