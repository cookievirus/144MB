#include "sort.h"

static SortRow g_scratch[ITEM_COUNT];

SortRow *SortScratch(void) { return g_scratch; }

void SortReset(SortState *s)
{
    s->mode = SORT_NONE;
    s->desc = 0;
}

void SortPress(SortState *s, SortMode mode)
{
    if (s->mode == (unsigned char)mode) {
        s->desc = (unsigned char)!s->desc;
        return;
    }
    s->mode = (unsigned char)mode;
    /* First press means the useful direction: best gear, biggest pile, top
       of the alphabet. */
    s->desc = (mode != SORT_ALPHA);
}

/* The tag sits between SORT_TAG_X and the purse, so the longest label is
   bounded by the same arithmetic as the hint line. Checked here rather than
   trusted, because these strings are edited far more often than the layout. */
#define SORT_TAG_MAX_CHARS 16
_Static_assert(sizeof("RARITY HIGH") - 1 <= SORT_TAG_MAX_CHARS, "sort tag too wide");
_Static_assert(sizeof("QTY LEAST")   - 1 <= SORT_TAG_MAX_CHARS, "sort tag too wide");
_Static_assert(sizeof("NAME Z-A")    - 1 <= SORT_TAG_MAX_CHARS, "sort tag too wide");
_Static_assert(sizeof("DEFAULT")     - 1 <= SORT_TAG_MAX_CHARS, "sort tag too wide");

const char *SortLabel(const SortState *s)
{
    switch (s->mode) {
    case SORT_RARITY: return s->desc ? "RARITY HIGH" : "RARITY LOW";
    case SORT_QTY:    return s->desc ? "QTY MOST"    : "QTY LEAST";
    case SORT_ALPHA:  return s->desc ? "NAME Z-A"    : "NAME A-Z";
    default:          return "DEFAULT";
    }
}

/* Fixed-point free, libc free: the only string work is this comparison. */
static int NameCmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

/* Negative when `a` should sit above `b`. */
static int RowCmp(const SortState *s, const SortRow *a, const SortRow *b)
{
    int d = 0;
    switch (s->mode) {
    case SORT_RARITY: d = (int)a->rarity - (int)b->rarity; break;
    case SORT_QTY:    d = (int)a->qty - (int)b->qty;       break;
    default:          break;
    }
    if (s->mode != SORT_ALPHA && d != 0) return s->desc ? -d : d;

    /* Alphabetical is both a mode and the tie-break for the other two, so a
       run of Common items is never left in table order looking unsorted. */
    d = NameCmp(a->name, b->name);
    return (s->mode == SORT_ALPHA && s->desc) ? -d : d;
}

void SortApply(const SortState *s, SortRow *rows, int n)
{
    if (s->mode == SORT_NONE) return;

    for (int i = 1; i < n; i++) {
        const SortRow key = rows[i];
        int j = i - 1;
        while (j >= 0 && RowCmp(s, &key, &rows[j]) < 0) {
            rows[j + 1] = rows[j];
            j--;
        }
        rows[j + 1] = key;
    }
}
