/* ui.h - primitives shared by every panel: the notched panel and the palette.

   Every widget in the game is drawn from rectangles. A nine-slice panel would
   cost ~288 bytes of asset data and a baked one ~12 KB, and at 320x240 a
   corner radius cannot exceed one pixel, so neither buys anything. */
#ifndef UI_H
#define UI_H

#include "gfx.h"
#include "rarity.h"
#include "ui_font.h"

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

/* ---- full-page screen furniture ----------------------------------------

   Lifted out of ui_menu.c in 1.2 so the shop draws the same frame as the
   inventory instead of a near-copy of it. Two callers sharing one routine is
   the cheap direction here: the duplicate was ~600 bytes of near-identical
   drawing code, and the real cost of the copy would have been the two
   layouts drifting a pixel apart every time one was touched. */

#define PAGE_X   8
#define PAGE_Y   8
#define PAGE_W 304
#define PAGE_H 224

#define LIST_X   14
#define LIST_W  148
#define DETAIL_X 168
#define DETAIL_W 138          /* 23 columns at 6 px per glyph */
#define BODY_Y   34
#define ROW_H    11
#define ROW_MARK_X  2         /* cursor caret       */
#define ROW_BALL_X  9         /* quality ball       */
#define ROW_TEXT_X 19         /* label, clear of both */
#define LIST_ROWS 14
#define HINT_Y  219

/* Foot of the detail pane. UiDetail's hand-wrapped body runs to roughly
   y=125 at five lines, so numeric stats sit here rather than above it where
   they would collide with the rule. */
#define DETAIL_STAT_Y 178

/* Panel, title, rules and the key hint along the bottom. */
void UiPageChrome(const char *title, const char *hint);

/* The hint line runs off the right edge of the panel if it is too long, and
   there is no wrapping to catch it: UiDrawText walks a fixed 6 px pitch until
   the string ends, so the overflow is drawn straight over the backdrop. 1.3
   shipped exactly that - "ESC LEAVE" came out as "ESC LEA" and the tail sat
   outside the frame.
 
   Clipping it at draw time would hide the bug rather than fix it, and a
   runtime length check costs a branch every frame to catch a mistake that is
   fully known at compile time. So the budget is a constant and every hint is
   asserted against it where it is defined. Adding one word too many is now a
   build error naming the offending string. */
/* Where a screen puts its sort tag: clear of the longest page title
   ("INVENTORY", "EQUIPMENT", "ITEM SHOP" are all 9 characters = 54 px from
   LIST_X), and on the left rather than tucked beside the shop's purse.

   Right-aligning it next to the gold was the obvious layout and it was wrong:
   the purse is drawn right-aligned too, so the two slide toward each other as
   the player gets richer and collide at five digits. Nothing in the demo
   reaches five digits today, which is exactly why it would have shipped. */
#define SORT_TAG_X (LIST_X + 62)

#define HINT_MAX_CHARS (((PAGE_X + PAGE_W) - LIST_X - 4) / FONT_CELL_W)

#define UI_HINT_FITS(s) \
    _Static_assert(sizeof(s) - 1 <= HINT_MAX_CHARS, \
                   "hint line is wider than the panel: " s)

/* One list row. Pass rarity < 0 for a row with no quality tier.

   Tiered rows carry their colour whether or not they are selected. Dimming
   every unselected row is the usual way to show focus, but it would drain the
   colour out of a whole list to mark one line, and being scannable at a
   glance is the entire point of the ball. Focus is left to the bar and the
   caret, which do not compete with hue. */
/* Pixels kept clear at the right of a row for its number column. A row label
   is clipped to what is left, with the last character replaced by a full stop
   so the player can see that a name was cut rather than misread it.

   Clipping rather than widening the list: the detail pane is 23 columns wide
   and every item description in game_data.h is hand-wrapped to exactly that,
   so taking pixels from it would mean rewrapping thirty-odd strings and
   living with whatever bad breaks fell out. The full name is always on screen
   anyway - it is the heading of the detail pane for the selected row. */
#define ROW_QTY_RESERVE   34   /* 5 digits + gap  */
#define ROW_MONEY_RESERVE 40   /* "9999 G" + gap  */

void UiRow(int x, int y, int w, const char *label, bool selected, int rarity,
           int reserve);

/* Right-aligned unsigned decimal. No printf: pulling in stdio's formatter for
   two call sites costs far more than sixteen bytes of division. */
void UiNumber(int right, int y, int value, Color tint);

/* Right-aligned price with its unit: "5 G". The suffix is per row rather than
   a column heading because the shop puts prices and stock counts in the same
   column position on different tabs, and an unlabelled 5 next to POTION reads
   as "five potions" at least once to everybody. */
void UiMoney(int right, int y, int value, Color tint);

/* Inset detail pane: heading, optional tier line, optional sub-line, rule,
   hand-wrapped body. */
void UiDetail(const char *heading, const char *sub, const char *body,
              int rarity);

#endif /* UI_H */
