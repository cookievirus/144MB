/* inventory.h - the player's mutable state.

   ITEMS[].qty in game_data.h is `const` and stays that way: it is the seed
   the game starts from, not the running total. Everything that changes lives
   here.

   This is a single global rather than a struct threaded through every draw
   call. ui_menu.c and shop.c already read global const tables, so reading one
   global mutable struct keeps them consistent, and threading a pointer
   through eight drawing functions would cost stack traffic on every frame to
   express something the game only ever has one of. The cost is that the
   inventory cannot be snapshotted for an undo; if that is ever wanted, it
   becomes a struct and the accessors below are already the seam to change.

   Data cost: 24 held counts x 2 bytes + one int = 52 bytes of .bss. */
#ifndef INVENTORY_H
#define INVENTORY_H

#include "game_data.h"

typedef struct Inventory {
    unsigned short held[ITEM_COUNT];
    int gold;
} Inventory;

extern Inventory g_inv;

void InvReset(void);

int InvHeld(int item);
int InvGold(void);

/* What the trader asks for one unit of a stock row, and what he pays for one
   unit out of the player's pack. Both go through RarityScaleValue, so the
   tier ladder drives the whole economy from one table. */
int InvBuyPrice(int stock_row);
int InvSellPrice(int item);

/* Both are all-or-nothing: they return false and change nothing if the coin
   is short or the pack is empty, so a caller cannot half-complete a trade. */
bool InvBuy(int item, int price);
bool InvSell(int item, int price);

#endif /* INVENTORY_H */
