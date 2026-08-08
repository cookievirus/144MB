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

   Data cost: 27 held counts x 2 bytes + one int + one mask = 62 bytes of
   .bss. 1.5 added three forgeable gear rows and the known-recipe mask. */
#ifndef INVENTORY_H
#define INVENTORY_H

#include "game_data.h"

typedef struct Inventory {
    unsigned short held[ITEM_COUNT];
    int gold;
    /* Which recipes the player has learned. One bit each, and it lives here
       rather than in UiForge because it is player state that outlives the
       screen: the Blueprints menu reads it, the save blob writes it, and the
       forge screen is closed most of the time. */
    unsigned int known;
} Inventory;

extern Inventory g_inv;

void InvReset(void);

int InvHeld(int item);
int InvGold(void);

/* ---- recipes ----------------------------------------------------------- */

bool InvKnows(int recipe);
void InvLearn(int recipe);

/* ---- the forge --------------------------------------------------------- */

/* True when every ingredient is on the shelf. The forge asks this twice per
   frame per visible row, which is a few dozen byte comparisons - cheaper than
   any cache that would have to be invalidated on every trade. */
bool InvCanForge(int recipe);

/* 1.6 split what 1.5 called InvForge into its two halves, because the QTE
   sits between them: the ore goes into the fire when the minigame starts and
   the blade only exists if it finishes well. A ruined heat is exactly a spend
   with no matching grant, which is the whole stake of the minigame.

   InvSpendMaterials is all-or-nothing, like InvBuy and InvSell: it returns
   false and changes nothing if a single ingredient is short, so a caller
   cannot half-consume a recipe and leave the player with neither the ore nor
   the blade. */
bool InvSpendMaterials(int recipe);
bool InvGrantItem(int item);

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
