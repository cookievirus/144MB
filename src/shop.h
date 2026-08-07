/* shop.h - the trader's counter: two tabs, one list, one detail pane.

   Deliberately not a UiMenu screen. The menu's frames are a navigation stack
   over static tables; the shop mutates world state and needs its own cursor
   to survive a tab switch, its own per-visit stock and a result line. Pushing
   it onto the menu stack would have meant teaching MenuFrame about all three
   for the benefit of one screen.

   No confirmation prompt per unit. A unit is bought or sold per key press and
   the result line says what happened, which is one key press to undo at the
   spread. A modal per unit would triple the presses for a shop where the
   common action is buying eight of something cheap. The spread is the cost of
   a mistake, and it is meant to be felt rather than guarded against.

   Leaving the counter is confirmed, though, and that is not a contradiction:
   a mis-bought loaf costs four coin, and a counter closed by a reflexive ESC
   costs the walk back. The scene owns that exchange, since ESC is the scene's
   ladder and the question is JACK's to ask. */
#ifndef SHOP_H
#define SHOP_H

#include "ui.h"
#include "inventory.h"
#include "sort.h"

typedef enum ShopTab {
    SHOP_BUY = 0,
    SHOP_SELL,
    SHOP_TAB_COUNT
} ShopTab;

typedef struct UiShop {
    unsigned char stock[STOCK_COUNT];  /* units left this visit */
    signed char tab;
    signed char cursor;
    signed char scroll;
    SortState sort;
    bool open;
    const char *result;                /* short line under the list */
    float result_time;
} UiShop;

void ShopInit(UiShop *s);          /* also restocks */
void ShopRestock(UiShop *s);

bool ShopIsOpen(const UiShop *s);

/* A fresh visit: tab, cursor and sort all go back to the top. */
void ShopOpen(UiShop *s);

/* Coming back after "anything else?". Everything the player set up is still
   there, because from their side they never left the counter - JACK just
   asked a question over the top of it. */
void ShopResume(UiShop *s);

void ShopClose(UiShop *s);

void ShopUpdate(UiShop *s, float dt);

/* One input step. dx switches tab, dy moves the cursor, accept trades. */
void ShopInput(UiShop *s, int dx, int dy, bool accept);

void ShopSort(UiShop *s, SortMode mode);

void ShopDraw(const UiShop *s);

#endif /* SHOP_H */
