/* sort.h - list ordering for every screen that shows items.

   Three keys, each a toggle: pressing the key that is already active flips
   the direction rather than doing nothing. That is the whole interaction, so
   there is no sort menu, no cursor to lose and no state to explain.

     R  rarity      high -> low, then low -> high
     T  quantity    many -> few, then few -> many
     A  alphabetical  A -> Z, then Z -> A

   The default direction differs per mode because "first press" should mean
   the useful thing: the best gear, the biggest pile, the top of the alphabet.

   Sorting is an insertion sort over at most a couple of dozen rows rather
   than qsort. qsort costs a function-pointer call per comparison and drags
   in libc's implementation; insertion sort is about forty bytes of code and
   is faster at this size anyway. It is also stable, so the alphabetical
   tie-break below survives.

   Every mode tie-breaks alphabetically. Without it, two Common items would
   sit in whatever order the table happened to list them and the list would
   look unsorted to the player even though it is. */
#ifndef SORT_H
#define SORT_H

#include "game_data.h"

typedef enum SortMode {
    SORT_NONE = 0,
    SORT_RARITY,
    SORT_QTY,
    SORT_ALPHA
} SortMode;

typedef struct SortState {
    unsigned char mode;
    unsigned char desc;
} SortState;

/* One visible row, flattened so the sort does not need to know whether it is
   looking at an inventory category, a stock list or the player's pack.
   `ref` is the caller's own index - an ItemId or a SHOP_STOCK row - and is
   what the caller reads back after sorting. */
typedef struct SortRow {
    const char *name;
    unsigned short qty;
    unsigned char rarity;
    unsigned char ref;
} SortRow;

void SortReset(SortState *s);

/* A key press. Same mode again flips the direction. */
void SortPress(SortState *s, SortMode mode);

/* Short label for the screen chrome, e.g. "RARITY v". Never NULL. */
const char *SortLabel(const SortState *s);

void SortApply(const SortState *s, SortRow *rows, int n);

/* Scratch shared by every list screen: only one is ever on top, so one
   buffer serves them all rather than each carrying its own copy. */
SortRow *SortScratch(void);

#endif /* SORT_H */
