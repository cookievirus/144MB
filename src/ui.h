/* ui.h - primitives shared by every panel: the notched panel and the palette.

   Every widget in the game is drawn from rectangles. A nine-slice panel would
   cost ~288 bytes of asset data and a baked one ~12 KB, and at 320x240 a
   corner radius cannot exceed one pixel, so neither buys anything. */
#ifndef UI_H
#define UI_H

#include "gfx.h"

extern const Color UI_FILL;    /* translucent panel body   */
extern const Color UI_EDGE;    /* panel border             */
extern const Color UI_PLATE;   /* name plate / active tab  */
extern const Color UI_SELECT;  /* cursor highlight bar     */
extern const Color UI_TEXT;    /* primary text             */
extern const Color UI_DIM;     /* secondary text, hints    */
extern const Color UI_SHADE;   /* inset wells              */

/* Rectangle with its four corner pixels cut away. */
void UiPanel(int x, int y, int w, int h, Color fill, Color edge);

/* Horizontal rule, used to separate a panel's title from its body. */
void UiRule(int x, int y, int w, Color c);

#endif /* UI_H */
