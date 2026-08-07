#include "inventory.h"

Inventory g_inv;

void InvReset(void)
{
    for (int i = 0; i < ITEM_COUNT; i++) g_inv.held[i] = ITEMS[i].qty;
    g_inv.gold = GOLD_START;
}

int InvHeld(int item)
{
    if (item < 0 || item >= ITEM_COUNT) return 0;
    return (int)g_inv.held[item];
}

int InvGold(void) { return g_inv.gold; }

int InvBuyPrice(int stock_row)
{
    const StockRow *r = &SHOP_STOCK[stock_row];
    const int price = RarityScaleValue((int)r->base,
                                       (Rarity)ITEMS[r->item].rarity);
    /* A tier multiplier of x0.25 can round a cheap Junk row to nothing, and a
       free item is a coin printer rather than a bargain. */
    return (price < 1) ? 1 : price;
}

int InvSellPrice(int item)
{
    /* Sell price is derived from the same base the shop would quote, so an
       item the shop does not stock still has a sane value. Items outside the
       stock list fall back to their tier multiplier against a flat base of 8,
       which is deliberately mean: the trader has no buyer lined up. */
    int base = 8;
    for (int i = 0; i < STOCK_COUNT; i++) {
        if (SHOP_STOCK[i].item == (unsigned char)item) {
            base = (int)SHOP_STOCK[i].base;
            break;
        }
    }
    const int full = RarityScaleValue(base, (Rarity)ITEMS[item].rarity);
    const int paid = (full * SELL_NUMERATOR) / SELL_DENOMINATOR;
    return (paid < 1) ? 1 : paid;
}

bool InvBuy(int item, int price)
{
    if (item < 0 || item >= ITEM_COUNT) return false;
    if (price > g_inv.gold) return false;
    if (g_inv.held[item] >= 0xFFFF) return false;

    g_inv.gold -= price;
    g_inv.held[item]++;
    return true;
}

bool InvSell(int item, int price)
{
    if (item < 0 || item >= ITEM_COUNT) return false;
    if (g_inv.held[item] == 0) return false;

    g_inv.held[item]--;
    g_inv.gold += price;
    return true;
}
