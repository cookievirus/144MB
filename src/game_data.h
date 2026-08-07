/* game_data.h - static content tables.

   Descriptions are hand-wrapped to the 23-column detail pane. No word-wrap
   routine exists on purpose: at this table size, wrapping is cheaper as
   authored newlines than as code. Past roughly fifty entries that stops being
   true and a wrapper earns its ~150 bytes.

   Every row carries a quality tier. The field is a byte that lands inside
   padding that already existed, so ItemDef and EquipSlot are the same size
   they were before. */
#ifndef GAME_DATA_H
#define GAME_DATA_H

#include "rarity.h"

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
    unsigned char rarity;
    unsigned short qty;
} ItemDef;

static const char *const CAT_NAMES[CAT_COUNT] = { "MATERIALS", "GEAR", "ITEMS" };

/* Symbolic row indices. The shop, and eventually every recipe and quest,
   references items by index rather than by string compare. The enum mirrors
   the table below by hand, so the static assert at the foot of the table is
   what stops the two drifting apart. */
typedef enum ItemId {
    ITM_IRON_ORE = 0, ITM_COAL,        ITM_SLAG_CAKE,    ITM_OAK_CHARCOAL,
    ITM_SILVER_INGOT, ITM_BEAST_BONE,  ITM_QUENCH_OIL,   ITM_LEATHER_STRIP,
    ITM_FROST_BILLET, ITM_DRAGON_SCALE, ITM_STARFALL,    ITM_VOID_CINDER,
    ITM_SHORTSWORD,   ITM_FALCHION,    ITM_LONGSWORD,    ITM_BUCKLER,
    ITM_CHAIN_COIF,   ITM_WARHAMMER,   ITM_WIDOWMAKER,
    ITM_SMALL_POTION, ITM_STAMINA,     ITM_HARD_BREAD,   ITM_SALTED_MEAT,
    ITM_WHETSTONE,
    ITM_COUNT
} ItemId;

/* Starting quantities. Live counts are held by inventory.c from here on;
   this table is the seed, not the truth. */
static const ItemDef ITEMS[] = {
{ "Iron Ore", "Raw ore from the Ore\nRoad seams. Two parts\nore, one part slag,\nand no way to know\nwhich until it melts.", CAT_MATERIAL, RARITY_COMMON, 34 },
{ "Coal", "Burns hot and dirty.\nCheap enough that no\nsmith bothers to\ncount it.", CAT_MATERIAL, RARITY_COMMON, 112 },
{ "Slag Cake", "What the crucible\nrejects. Sold by the\nbarrow to men who do\nnot ask questions.", CAT_MATERIAL, RARITY_JUNK, 41 },
{ "Oak Charcoal", "Burns cleaner than\ncoal and costs four\ntimes as much. Worth\nit for edged work.", CAT_MATERIAL, RARITY_UNCOMMON, 18 },
{ "Silver Ingot", "Soft, bright, and\nuseless in a blade.\nBuyers pay for the\nshine, not the edge.", CAT_MATERIAL, RARITY_RARE, 3 },
{ "Beast Bone", "Ground into the\nquench, it hardens a\nsurface. Nobody\nagrees on why.", CAT_MATERIAL, RARITY_UNCOMMON, 7 },
{ "Quench Oil", "Slower than water and\nfar less likely to\ncrack a finished\nblade in half.", CAT_MATERIAL, RARITY_COMMON, 5 },
{ "Leather Strip", "Grip wrap. The first\nthing to wear out and\nthe cheapest thing to\nreplace.", CAT_MATERIAL, RARITY_COMMON, 26 },
{ "Frost-Iron Billet", "Cold to work and\ncolder to quench. One\nbillet, one blade, no\nsecond attempt.", CAT_MATERIAL, RARITY_EPIC, 1 },
{ "Dragon Scale", "Will not melt. Will\nnot yield. Not for\nthis forge, not in\nthis lifetime.", CAT_MATERIAL, RARITY_LEGENDARY, 1 },
{ "Starfall Shard", "Fell in grandfather's\ntime. Nobody living\nhas worked one, and\nthe records lie.", CAT_MATERIAL, RARITY_MYTH, 1 },
{ "Void Cinder", "Cold to the touch.\nThe coals dim when it\ncomes near them.", CAT_MATERIAL, RARITY_CURSED, 2 },

{ "Iron Shortsword", "Honest work. It will\nnot win a duel, but\nit will not fold in\nthe first hour.", CAT_GEAR, RARITY_COMMON, 4 },
{ "Notched Falchion", "Returned twice. The\nedge is a saw now and\nthe buyer knew it.", CAT_GEAR, RARITY_JUNK, 3 },
{ "Steel Longsword", "Folded seven times.\nThe party that buys\nthis one will come\nback for another.", CAT_GEAR, RARITY_UNCOMMON, 1 },
{ "Buckler", "Small, light, and\nmeant for a fighter\nwho intends to keep\nmoving.", CAT_GEAR, RARITY_COMMON, 2 },
{ "Chain Coif", "Two weeks of rings.\nPriced accordingly,\nand still nobody\nthinks it is enough.", CAT_GEAR, RARITY_UNCOMMON, 1 },
{ "Warhammer", "Not elegant. Solves\nthe specific problem\nof armour, and\nnothing else.", CAT_GEAR, RARITY_RARE, 1 },
{ "Widowmaker", "It sells for a\nfortune. Ask who\nowned it before, then\nask where he is.", CAT_GEAR, RARITY_CURSED, 1 },

{ "Small Potion", "Closes what a blade\nopens. Mostly.", CAT_ITEM, RARITY_UNCOMMON, 6 },
{ "Stamina Draught", "Bitter. Buys an hour\nat the anvil that the\nbody did not want to\ngive.", CAT_ITEM, RARITY_UNCOMMON, 2 },
{ "Hard Bread", "Keeps for a month.\nTastes like it.", CAT_ITEM, RARITY_COMMON, 9 },
{ "Salted Meat", "Trail food. Every\nparty leaves with it\nand nobody thanks you\nfor it.", CAT_ITEM, RARITY_COMMON, 4 },
{ "Whetstone", "A blade sold dull is\na blade returned.", CAT_ITEM, RARITY_COMMON, 3 },
};
#define ITEM_COUNT ((int)(sizeof(ITEMS) / sizeof(ITEMS[0])))

_Static_assert(ITEM_COUNT == ITM_COUNT,
               "ItemId and ITEMS have drifted apart - fix the enum");

typedef struct EquipSlot {
    const char *slot;
    const char *fitted;      /* NULL for an empty slot */
    const char *desc;
    unsigned char rarity;    /* ignored when fitted is NULL */
} EquipSlot;

static const EquipSlot EQUIPMENT[] = {
{ "HAMMER",  "Worn Cross-Peen", "Grandfather's. The\nhandle has been\nreplaced four times,\nthe head never.", RARITY_RARE },
{ "TONGS",   "Wolf-Jaw Tongs",  "Grips flat stock and\nround stock equally\nbadly. Still the pair\nreached for first.", RARITY_COMMON },
{ "ANVIL",   "Cracked Horn",    "The horn split in a\nwinter frost. Every\ncurve drawn on it now\nremembers the crack.", RARITY_JUNK },
{ "APRON",   "Scarred Leather", "Sixty burns and one\nsingle hole. Bragged\nabout more than it\ndeserves.", RARITY_COMMON },
{ "BELLOWS", "Twin Chamber",    "Steadier air than a\nsingle chamber, which\nmeans a steadier\nweld.", RARITY_UNCOMMON },
{ "GLOVES",  NULL,              "Nothing fitted.\nSpeed for skin: the\ntrade every smith\nmakes at least once.", RARITY_COMMON },
};
#define EQUIP_COUNT ((int)(sizeof(EQUIPMENT) / sizeof(EQUIPMENT[0])))

/* SCENE_NONE marks a destination the player cannot yet stand in. It is a
   separate field from `reachable` on purpose: a place can be closed for
   story reasons while its room already exists, and a place can be open on
   the map while its art has not been drawn. */
#define SCENE_NONE 0xFF

typedef struct Destination {
    const char *name;
    const char *desc;
    unsigned char reachable;
    unsigned char scene;      /* SceneId, or SCENE_NONE */
} Destination;

static const Destination DESTINATIONS[] = {
{ "The Smithy",       "You are here. The\nforge is lit and the\ncoal will not last\nthe week.", 1, 0 },
{ "Market Row",       "Ore, coal, and men\nwho know exactly what\nyour last commission\nsold for.", 1, 1 },
{ "Adventurers Guild","Where the parties\npost what they need\nand what they are\nwilling to pay.", 0, SCENE_NONE },
{ "Ore Road",         "Three days out. The\nseams are thin now\nand the escort is not\ncheap.", 0, SCENE_NONE },
{ "Capital Gate",     "Closed to a smith\nwithout a charter.\nEarn one, or find\nsomeone who has.", 0, SCENE_NONE },
};
#define DEST_COUNT ((int)(sizeof(DESTINATIONS) / sizeof(DESTINATIONS[0])))


/* ---- shop stock -------------------------------------------------------- */

/* A stock row is three bytes: which item, how many the trader will part with
   today, and the base price before the quality multiplier. The name, tier and
   description all live in ITEMS already, so repeating them here would cost
   pointers in .rodata to say what one byte already says.

   The quoted price is RarityScaleValue(base, tier), which is the first real
   use of the eighths table shipped unused in 1.1. A Rare Silver Ingot at base
   40 therefore quotes 100, and retuning a whole tier's economy is a one-byte
   edit in RARITY_MUL rather than a pass over every row. */
typedef struct StockRow {
    unsigned char item;      /* ItemId                     */
    unsigned char stock;     /* units available this visit */
    unsigned short base;     /* coin, before the tier multiplier */
} StockRow;

static const StockRow SHOP_STOCK[] = {
    { ITM_SMALL_POTION,  8, 22 },
    { ITM_STAMINA,       4, 30 },
    { ITM_HARD_BREAD,   20,  4 },
    { ITM_SALTED_MEAT,  12,  9 },
    { ITM_WHETSTONE,     6, 12 },
    { ITM_QUENCH_OIL,    5, 18 },
    { ITM_LEATHER_STRIP,30,  3 },
    { ITM_COAL,         99,  2 },
    { ITM_OAK_CHARCOAL, 24,  6 },
    { ITM_IRON_ORE,     60,  5 },
    { ITM_BEAST_BONE,    9, 14 },
    { ITM_SILVER_INGOT,  2, 40 },
    { ITM_BUCKLER,       1, 60 },
    { ITM_CHAIN_COIF,    1, 90 },
};
#define STOCK_COUNT ((int)(sizeof(SHOP_STOCK) / sizeof(SHOP_STOCK[0])))

/* The trader buys back at half. A single rate rather than a per-item spread:
   the interesting decision in this game is what to forge, not which shop to
   dump surplus charcoal at. */
#define SELL_NUMERATOR   1
#define SELL_DENOMINATOR 2

#define GOLD_START 250

#endif /* GAME_DATA_H */
