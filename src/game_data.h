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
    ITM_DAGGER,       ITM_ASH_STAFF,   ITM_CUIRASS,
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

/* 1.5: the three pieces the forge can make that nothing else in the game
   sells. Seeded at zero - the only way to hold one is to have made it. */
{ "Iron Dagger", "Quick to make and\nquicker to lose. Every\nparty buys three.", CAT_GEAR, RARITY_COMMON, 0 },
{ "Ash Staff", "Bone core, silver\ncollar, ash shaft. The\nguild pays for the\ncollar and nothing\nelse.", CAT_GEAR, RARITY_RARE, 0 },
{ "Iron Cuirass", "Forty lumps of ore and\na week of hammering.\nPriced like it, and\nstill argued over.", CAT_GEAR, RARITY_UNCOMMON, 0 },

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

/* ---- what a room lets you do -------------------------------------------

   1.7 folded SceneDef's has_shop and has_forge into one field. Two booleans
   said a room could have a counter and an anvil at once, which no room does
   and which the main menu could not have drawn anyway: the root grid has one
   slot for what the room is for. A byte that names the single feature is
   smaller, and it is what the menu actually needs to label a row.

   The label is what appears in the menu, so it is here beside the other
   content tables rather than in scene.h - ui_menu.c must reach it and does
   not include scene.h. */

typedef enum RoomFeature {
    ROOM_FEATURE_NONE = 0,
    ROOM_FEATURE_TRADE,
    ROOM_FEATURE_FORGE,
    ROOM_FEATURE_PARTY,
    ROOM_FEATURE_COUNT
} RoomFeature;

/* NULL for NONE: a room with no feature contributes no row, and a label that
   would never be drawn is a label that would silently rot. */
static const char *const ROOM_FEATURE_LABELS[ROOM_FEATURE_COUNT] = {
    NULL, "BUY/SELL", "FORGE", "PARTY"
};

/* PARTY has no room yet. The Guild needs a backdrop, and the .gitignore in
   this repo is explicit that a placeholder must never reach a submission - so
   the room waits on art, not on code. When it arrives it is one byte in its
   SceneDef and one case in the scene's feature switch; the label and the menu
   row already work. */

/* ---- forge recipes -----------------------------------------------------

   A recipe is eight bytes: what it makes, which shelf of the forge menu it
   sits on, and up to three ingredients as {ItemId, count} pairs.

   The pairs are two bytes rather than one packed byte. Nibble-packing an
   ingredient - item id in the high nibble, count in the low - is the obvious
   saving and it does not survive contact with this table: there are more than
   fifteen items and the Cuirass wants forty ore. Four ingredient bytes per
   recipe would buy 24 bytes across the whole table and cap the design at
   fifteen materials in fifteens, which is the wrong trade this early.

   Everything else the menu needs - name, tier, description, sell price - is
   already in ITEMS and is reached through `out`. A recipe that repeated any
   of it would cost pointers in .rodata to say what one byte already says,
   which is the same call StockRow makes. */

typedef enum ForgeCat {
    FORGE_WEAPON = 0,
    FORGE_ARMOR,
    FORGE_CAT_COUNT
} ForgeCat;

static const char *const FORGE_CAT_NAMES[FORGE_CAT_COUNT] = { "WEAPONS", "ARMOR" };

/* Singular, for the detail pane's sub-line. */
static const char *const FORGE_CAT_ONE[FORGE_CAT_COUNT] = { "WEAPON", "ARMOR" };

#define RECIPE_SLOTS 3
#define RECIPE_NONE  0xFF

typedef struct RecipeMat {
    unsigned char item;      /* ItemId, or RECIPE_NONE for an unused slot */
    unsigned char qty;
} RecipeMat;

typedef struct RecipeDef {
    unsigned char out;       /* ItemId produced on a clean heat   */
    unsigned char cat;       /* ForgeCat                          */
    /* 1.6: what a flawless heat produces instead, or RECIPE_NONE for a recipe
       with no better version of itself. It is a whole ItemId rather than a
       tier bump because held items are counts, not instances - the pack knows
       it holds four of item 12, and there is nowhere to record that one of
       them came out better than the others. Promoting the output is the only
       way to say "this one is finer" in a model built on counts, and it costs
       one byte per recipe against the several hundred that per-instance
       quality would cost in the save blob alone. */
    unsigned char fine;
    RecipeMat mat[RECIPE_SLOTS];
} RecipeDef;                 /* 9 bytes */

#define MAT_END { RECIPE_NONE, 0 }

static const RecipeDef RECIPES[] = {
/*               output          shelf         fine            ingredients */
{ ITM_SHORTSWORD, FORGE_WEAPON, ITM_LONGSWORD, { { ITM_IRON_ORE,  6 }, { ITM_COAL,          4 }, { ITM_LEATHER_STRIP, 1 } } },
{ ITM_DAGGER,     FORGE_WEAPON, RECIPE_NONE,   { { ITM_IRON_ORE,  2 }, { ITM_LEATHER_STRIP, 1 }, MAT_END                  } },
{ ITM_ASH_STAFF,  FORGE_WEAPON, RECIPE_NONE,   { { ITM_BEAST_BONE,2 }, { ITM_SILVER_INGOT,  1 }, { ITM_LEATHER_STRIP, 1 } } },
{ ITM_CUIRASS,    FORGE_ARMOR,  RECIPE_NONE,   { { ITM_IRON_ORE, 40 }, { ITM_COAL,         20 }, { ITM_LEATHER_STRIP, 3 } } },
{ ITM_CHAIN_COIF, FORGE_ARMOR,  RECIPE_NONE,   { { ITM_IRON_ORE,  7 }, { ITM_COAL,          4 }, MAT_END                  } },
{ ITM_BUCKLER,    FORGE_ARMOR,  RECIPE_NONE,   { { ITM_IRON_ORE,  4 }, { ITM_OAK_CHARCOAL,  2 }, { ITM_LEATHER_STRIP, 2 } } },
};
#define RECIPE_COUNT ((int)(sizeof(RECIPES) / sizeof(RECIPES[0])))

/* Only the Shortsword has a finer version of itself today, because Steel
   Longsword was already in ITEMS and the other five have no counterpart that
   exists. The mechanism is one byte per recipe and one branch; filling the
   column in is a content task - five names, five descriptions, five tiers -
   and until it is done a flawless heat on a dagger is worth exactly the same
   as a scrappy one. That is a real gap and it is in the manual checklist. */

/* Which recipes the player knows is one 32-bit word in the save blob, so the
   table cannot outgrow it without the mask growing too. */
_Static_assert(RECIPE_COUNT <= 32, "RECIPES has outgrown the 32-bit known mask");

/* Every recipe the demo ships with is known from the start. The mask is not
   therefore pointless: it is what lets a recipe be *added* later without the
   Blueprints menu claiming the player has always had it, and it is already in
   the save blob, so teaching a recipe in Week 3 costs one bit rather than a
   save version. */
#define RECIPES_KNOWN_AT_START 0x3Fu

/* The trader buys back at half. A single rate rather than a per-item spread:
   the interesting decision in this game is what to forge, not which shop to
   dump surplus charcoal at. */
#define SELL_NUMERATOR   1
#define SELL_DENOMINATOR 2

#define GOLD_START 250

#endif /* GAME_DATA_H */
