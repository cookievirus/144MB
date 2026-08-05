/* ui_font.h - 5x7 bitmap font baked into a single texture at boot. */
#ifndef UI_FONT_H
#define UI_FONT_H

#include "gfx.h"

#include "../assets/font5x7.h"

/* Builds the glyph atlas. Must be called after InitWindow(). */
void UiFontLoad(void);
void UiFontUnload(void);

/* Draws at most `count` characters; pass a negative count for the whole
   string. Newlines start a new line at the original x. Returns the y of the
   line after the last one drawn. */
int UiDrawTextN(const char *text, int count, int x, int y, Color tint);
int UiDrawText(const char *text, int x, int y, Color tint);

int UiTextWidth(const char *text);

#endif /* UI_FONT_H */
