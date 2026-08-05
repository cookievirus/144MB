/* game_data.h - static content tables.

   Descriptions are hand-wrapped to the 23-column detail pane. No word-wrap
   routine exists on purpose: at this table size, wrapping is cheaper as
   authored newlines than as code. Past roughly fifty entries that stops being
   true and a wrapper earns its ~150 bytes. */
#ifndef GAME_DATA_H
#define GAME_DATA_H

typedef enum ItemCat {
    CAT_MATERIAL = 0,
    CAT_GEAR,
    CAT_ITEM,
    CAT_COUNT
} ItemCat;

typedef struct ItemDef {
    const char *name;
    const char *desc;
    unsigned char cat;
    unsigned short qty;
} ItemDef;

static const char *const CAT_NAMES[CAT_COUNT] = { "MATERIALS", "GEAR", "ITEMS" };

/* Quantities are baked in for the demo; the real build will read them from
   the player's inventory. */
static const ItemDef ITEMS[] = {
{ "Iron Ore", "Raw ore from the Ore\nRoad seams. Two parts\nore, one part slag,\nand no way to know\nwhich until it melts.", CAT_MATERIAL, 34 },
{ "Coal", "Burns hot and dirty.\nCheap enough that no\nsmith bothers to\ncount it.", CAT_MATERIAL, 112 },
{ "Oak Charcoal", "Burns cleaner than\ncoal and costs four\ntimes as much. Worth\nit for edged work.", CAT_MATERIAL, 18 },
{ "Silver Ingot", "Soft, bright, and\nuseless in a blade.\nBuyers pay for the\nshine, not the edge.", CAT_MATERIAL, 3 },
{ "Beast Bone", "Ground into the\nquench, it hardens a\nsurface. Nobody\nagrees on why.", CAT_MATERIAL, 7 },
{ "Quench Oil", "Slower than water and\nfar less likely to\ncrack a finished\nblade in half.", CAT_MATERIAL, 5 },
{ "Leather Strip", "Grip wrap. The first\nthing to wear out and\nthe cheapest thing to\nreplace.", CAT_MATERIAL, 26 },

{ "Iron Shortsword", "Honest work. It will\nnot win a duel, but\nit will not fold in\nthe first hour.", CAT_GEAR, 4 },
{ "Steel Longsword", "Folded seven times.\nThe party that buys\nthis one will come\nback for another.", CAT_GEAR, 1 },
{ "Buckler", "Small, light, and\nmeant for a fighter\nwho intends to keep\nmoving.", CAT_GEAR, 2 },
{ "Chain Coif", "Two weeks of rings.\nPriced accordingly,\nand still nobody\nthinks it is enough.", CAT_GEAR, 1 },
{ "Warhammer", "Not elegant. Solves\nthe specific problem\nof armour, and\nnothing else.", CAT_GEAR, 1 },

{ "Small Potion", "Closes what a blade\nopens. Mostly.", CAT_ITEM, 6 },
{ "Stamina Draught", "Bitter. Buys an hour\nat the anvil that the\nbody did not want to\ngive.", CAT_ITEM, 2 },
{ "Hard Bread", "Keeps for a month.\nTastes like it.", CAT_ITEM, 9 },
{ "Salted Meat", "Trail food. Every\nparty leaves with it\nand nobody thanks you\nfor it.", CAT_ITEM, 4 },
{ "Whetstone", "A blade sold dull is\na blade returned.", CAT_ITEM, 3 },
};
#define ITEM_COUNT ((int)(sizeof(ITEMS) / sizeof(ITEMS[0])))

typedef struct EquipSlot {
    const char *slot;
    const char *fitted;      /* NULL for an empty slot */
    const char *desc;
} EquipSlot;

static const EquipSlot EQUIPMENT[] = {
{ "HAMMER",  "Worn Cross-Peen", "Grandfather's. The\nhandle has been\nreplaced four times,\nthe head never." },
{ "TONGS",   "Wolf-Jaw Tongs",  "Grips flat stock and\nround stock equally\nbadly. Still the pair\nreached for first." },
{ "ANVIL",   "Cracked Horn",    "The horn split in a\nwinter frost. Every\ncurve drawn on it now\nremembers the crack." },
{ "APRON",   "Scarred Leather", "Sixty burns and one\nsingle hole. Bragged\nabout more than it\ndeserves." },
{ "BELLOWS", "Twin Chamber",    "Steadier air than a\nsingle chamber, which\nmeans a steadier\nweld." },
{ "GLOVES",  NULL,              "Nothing fitted.\nSpeed for skin: the\ntrade every smith\nmakes at least once." },
};
#define EQUIP_COUNT ((int)(sizeof(EQUIPMENT) / sizeof(EQUIPMENT[0])))

typedef struct Destination {
    const char *name;
    const char *desc;
    unsigned char reachable;
} Destination;

static const Destination DESTINATIONS[] = {
{ "The Smithy",       "You are here. The\nforge is lit and the\ncoal will not last\nthe week.", 0 },
{ "Market Row",       "Ore, coal, and men\nwho know exactly what\nyour last commission\nsold for.", 1 },
{ "Adventurers Guild","Where the parties\npost what they need\nand what they are\nwilling to pay.", 1 },
{ "Ore Road",         "Three days out. The\nseams are thin now\nand the escort is not\ncheap.", 1 },
{ "Capital Gate",     "Closed to a smith\nwithout a charter.\nEarn one, or find\nsomeone who has.", 0 },
};
#define DEST_COUNT ((int)(sizeof(DESTINATIONS) / sizeof(DESTINATIONS[0])))

#endif /* GAME_DATA_H */
