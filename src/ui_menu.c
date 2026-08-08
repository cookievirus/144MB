#include "ui_menu.h"

#include "ui_font.h"

/* ---- layout, in virtual-screen pixels ---------------------------------- */

#define ROOT_X   8
#define ROOT_Y   8
#define ROOT_W 142
#define ROOT_COLS 2
#define CELL_W   64
#define CELL_H   16
#define CELL_GAP  2
#define ROOT_PAD  8

/* The root grid is no longer a fixed 2x2. It carries the room's own feature
   when the room has one, so it is five rows or six, and its height follows.
   Every label fits CELL_W at 6 px per glyph with the 5 px inset: the longest
   is INVENTORY at 9 characters = 59 px. */
#define ROOT_MAX_ROWS 6
#define ROOT_HEIGHT(rows) (ROOT_PAD * 2 + (rows) * CELL_H + ((rows) - 1) * CELL_GAP)

typedef enum RootRow {
    ROOT_TALK = 0,
    ROOT_FEATURE,
    ROOT_INVENTORY,
    ROOT_EQUIPMENT,
    ROOT_MAP,
    ROOT_END_DAY,
    ROOT_ROW_COUNT
} RootRow;

static const char *const ROOT_LABELS[ROOT_ROW_COUNT] = {
    "TALK", NULL, "INVENTORY", "EQUIPMENT", "MAP", "END DAY"
};

_Static_assert(ROOT_ROW_COUNT <= ROOT_MAX_ROWS, "root grid has outgrown its panel");

/* Which rows this room shows, in order. The feature sits second rather than
   first: TALK is in the same place in every room, and a menu whose first
   entry moves depending on where you are standing is a menu you have to read
   before you can use it. */
static int RootBuild(const UiMenu *m, unsigned char *out)
{
    int n = 0;
    out[n++] = ROOT_TALK;
    if (m->feature != ROOM_FEATURE_NONE) out[n++] = ROOT_FEATURE;
    out[n++] = ROOT_INVENTORY;
    out[n++] = ROOT_EQUIPMENT;
    out[n++] = ROOT_MAP;
    out[n++] = ROOT_END_DAY;
    return n;
}

static const char *RootLabel(const UiMenu *m, int row)
{
    if (row == ROOT_FEATURE) return ROOM_FEATURE_LABELS[m->feature];
    return ROOT_LABELS[row];
}

#define HINT_LIST   "ARROWS MOVE   SPACE/ENTER SELECT   ESC BACK"
#define HINT_SORT   "R RARITY  T QTY  A NAME   ESC BACK"
#define HINT_TRAVEL "ARROWS MOVE   SPACE/ENTER TRAVEL   ESC BACK"

UI_HINT_FITS(HINT_LIST);
UI_HINT_FITS(HINT_SORT);
UI_HINT_FITS(HINT_TRAVEL);

/* ---- helpers ----------------------------------------------------------- */

static int Wrap(int v, int n) { return (v % n + n) % n; }

static int Clamp(int v, int lo, int hi)
{
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

/* Items are stored in one flat table and filtered on the fly rather than
   duplicated into per-category arrays: the walk is a few dozen comparisons on
   a key press, and the alternative costs pointer tables in .rodata forever.

   The filtered category is flattened into the shared sort scratch and ordered
   there, so the cursor, the rows and the detail pane all read the same list.
   Rebuilt on every call rather than cached: at 24 rows the walk is cheaper
   than the state needed to know when the cache went stale, and the held
   counts change under it every time the player buys something. */
static int BuildCat(const UiMenu *m, int cat)
{
    SortRow *rows = SortScratch();
    int n = 0;
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (ITEMS[i].cat != cat) continue;
        rows[n].name = ITEMS[i].name;
        rows[n].qty = (unsigned short)InvHeld(i);
        rows[n].rarity = ITEMS[i].rarity;
        rows[n].ref = (unsigned char)i;
        n++;
    }
    SortApply(&m->sort, rows, n);
    return n;
}

static int CatCount(int cat)
{
    int n = 0;
    for (int i = 0; i < ITEM_COUNT; i++) if (ITEMS[i].cat == cat) n++;
    return n;
}

/* Pass rarity < 0 for a row that has no quality tier, such as a destination.

   Tiered rows carry their colour whether or not they are selected. Dimming
   every unselected row is the usual trick for showing focus, but it would
   drain the colour out of eight rows to mark one, and being scannable at a
   glance is the entire point of the ball. Focus is left to the bar and the
   caret, which do not compete with hue. */




/* ---- lifecycle --------------------------------------------------------- */

static void Push(UiMenu *m, MenuScreen screen)
{
    if (m->depth >= MENU_MAX_DEPTH) return;
    MenuFrame *f = &m->stack[m->depth++];
    f->screen = (unsigned char)screen;
    f->cursor = 0;
    f->tab = 0;
    f->scroll = 0;
}

void UiMenuInit(UiMenu *m)
{
    m->depth = 0;
    m->feature = ROOM_FEATURE_NONE;
    SortReset(&m->sort);
}

bool UiMenuTakesSort(const UiMenu *m)
{
    return m->depth > 0 &&
           m->stack[m->depth - 1].screen == (unsigned char)SCREEN_INVENTORY;
}
bool UiMenuIsOpen(const UiMenu *m) { return m->depth > 0; }
void UiMenuClose(UiMenu *m) { m->depth = 0; }

void UiMenuOpen(UiMenu *m, int feature)
{
    m->depth = 0;
    m->feature = (unsigned char)((feature >= 0 && feature < ROOM_FEATURE_COUNT)
                                 ? feature : ROOM_FEATURE_NONE);
    Push(m, SCREEN_ROOT);
}

/* ---- input ------------------------------------------------------------- */

MenuCommand UiMenuInput(UiMenu *m, int dx, int dy, bool accept, bool back)
{
    if (m->depth == 0) return MENU_CMD_NONE;

    if (back) {
        m->depth--;
        return MENU_CMD_NONE;
    }

    MenuFrame *f = &m->stack[m->depth - 1];

    switch (f->screen) {
    case SCREEN_ROOT: {
        unsigned char rows[ROOT_MAX_ROWS];
        const int n = RootBuild(m, rows);
        const int grid_rows = (n + ROOT_COLS - 1) / ROOT_COLS;

        int col = f->cursor % ROOT_COLS;
        int row = f->cursor / ROOT_COLS;
        if (dx) col = Wrap(col + dx, ROOT_COLS);
        if (dy) row = Wrap(row + dy, grid_rows);
        /* The last grid row can be short. Landing on the empty half of it
           would put the caret on nothing, so the cursor clamps to the last
           real entry instead - the same rule the inventory list follows when
           a category runs out. */
        f->cursor = (signed char)Clamp(row * ROOT_COLS + col, 0, n - 1);

        if (accept) {
            switch (rows[f->cursor]) {
            case ROOT_TALK:      UiMenuClose(m); return MENU_CMD_TALK;
            case ROOT_FEATURE:   UiMenuClose(m); return MENU_CMD_FEATURE;
            case ROOT_INVENTORY: Push(m, SCREEN_INVENTORY); break;
            case ROOT_EQUIPMENT: Push(m, SCREEN_EQUIPMENT); break;
            case ROOT_MAP:       Push(m, SCREEN_MAP); break;
            /* Not closed here. The scene puts a confirmation up over the
               menu, and a player who says no should find the menu where they
               left it rather than back in the room. */
            default:             return MENU_CMD_END_DAY;
            }
        }
    } break;

    case SCREEN_INVENTORY: {
        if (dx) {
            f->tab = (signed char)Clamp(f->tab + dx, 0, CAT_COUNT - 1);
            f->cursor = 0;
            f->scroll = 0;
        }
        const int n = CatCount(f->tab);
        if (dy && n > 0) f->cursor = (signed char)Clamp(f->cursor + dy, 0, n - 1);

        /* Keep the cursor inside the visible window without recentring it. */
        if (f->cursor < f->scroll) f->scroll = f->cursor;
        if (f->cursor >= f->scroll + LIST_ROWS) {
            f->scroll = (signed char)(f->cursor - LIST_ROWS + 1);
        }
    } break;

    case SCREEN_EQUIPMENT: {
        int col = f->cursor % 2;
        int row = f->cursor / 2;
        if (dx) col = Wrap(col + dx, 2);
        if (dy) row = Wrap(row + dy, EQUIP_COUNT / 2);
        f->cursor = (signed char)(row * 2 + col);
    } break;

    default:
        if (dy) f->cursor = (signed char)Clamp(f->cursor + dy, 0, DEST_COUNT - 1);
        if (accept) {
            const Destination *d = &DESTINATIONS[f->cursor];
            /* The menu reports where the player wants to go and closes. It
               does not load a room: the scene owns world state, exactly as
               TALK already works. */
            if (d->reachable && d->scene != SCENE_NONE) {
                UiMenuClose(m);
                return (MenuCommand)(MENU_CMD_TRAVEL_BASE + d->scene);
            }
        }
        break;
    }

    return MENU_CMD_NONE;
}

/* ---- drawing ----------------------------------------------------------- */

static void DrawRoot(const UiMenu *m, const MenuFrame *f)
{
    unsigned char rows[ROOT_MAX_ROWS];
    const int n = RootBuild(m, rows);
    const int grid_rows = (n + ROOT_COLS - 1) / ROOT_COLS;

    UiPanel(ROOT_X, ROOT_Y, ROOT_W, ROOT_HEIGHT(grid_rows), UI_FILL, UI_EDGE);

    for (int i = 0; i < n; i++) {
        const int col = i % ROOT_COLS;
        const int row = i / ROOT_COLS;
        const int x = ROOT_X + ROOT_PAD + col * (CELL_W + CELL_GAP);
        const int y = ROOT_Y + ROOT_PAD + row * (CELL_H + CELL_GAP);
        const bool on = (i == f->cursor);

        if (on) UiPanel(x, y, CELL_W, CELL_H, UI_SELECT, UI_EDGE);
        UiDrawText(RootLabel(m, rows[i]), x + 5, y + 4, on ? UI_TEXT : UI_DIM);
    }
}

static void DrawInventory(const UiMenu *m, const MenuFrame *f)
{
    UiPageChrome("INVENTORY", HINT_SORT);

    /* The active ordering is shown on the title line. A sort the player
       cannot see is a sort they will assume is broken the first time two
       Common items sit next to each other. */
    UiDrawText(SortLabel(&m->sort), SORT_TAG_X, PAGE_Y + 7, UI_DIM);

    int tx = LIST_X;
    for (int c = 0; c < CAT_COUNT; c++) {
        const int w = UiTextWidth(CAT_NAMES[c]) + 10;
        const bool on = (c == f->tab);
        if (on) UiPanel(tx, BODY_Y - 2, w, 13, UI_PLATE, UI_EDGE);
        UiDrawText(CAT_NAMES[c], tx + 5, BODY_Y + 1, on ? UI_TEXT : UI_DIM);
        tx += w + 3;
    }

    const int n = BuildCat(m, f->tab);
    const SortRow *rows = SortScratch();

    for (int i = 0; i < LIST_ROWS; i++) {
        const int idx = f->scroll + i;
        if (idx >= n) break;
        const int y = BODY_Y + 20 + i * ROW_H;
        const bool on = (idx == f->cursor);
        UiRow(LIST_X, y, LIST_W, rows[idx].name, on, (int)rows[idx].rarity,
              ROW_COUNT_RESERVE);
        UiCount(LIST_X + LIST_W - 4, y, (int)rows[idx].qty,
                on ? UI_TEXT : UI_DIM);
    }

    if (n > 0) {
        const ItemDef *it = &ITEMS[rows[f->cursor].ref];
        UiDetail(it->name, NULL, it->desc, (int)it->rarity);
        UiRule(DETAIL_X, DETAIL_STAT_Y - 6, DETAIL_W, UI_EDGE);
        UiDrawText("HELD", DETAIL_X, DETAIL_STAT_Y, UI_DIM);
        UiCount(DETAIL_X + DETAIL_W, DETAIL_STAT_Y,
                (int)rows[f->cursor].qty, UI_TEXT);
    }
}

static void DrawEquipment(const MenuFrame *f)
{
    UiPageChrome("EQUIPMENT", HINT_LIST);

    for (int i = 0; i < EQUIP_COUNT; i++) {
        const int col = i % 2;
        const int row = i / 2;
        const int x = LIST_X + col * 76;
        const int y = BODY_Y + 4 + row * 28;
        const bool on = (i == f->cursor);

        UiPanel(x, y, 74, 24, on ? UI_SELECT : UI_SHADE, UI_EDGE);
        UiDrawText(EQUIPMENT[i].slot, x + 4, y + 4, on ? UI_TEXT : UI_DIM);

        /* The tier belongs to the fitted tool, not to the slot, so the ball
           sits on the status line and an empty slot shows none. */
        if (EQUIPMENT[i].fitted != NULL) {
            RarityBall((Rarity)EQUIPMENT[i].rarity, x + 4, y + 12);
            UiDrawText("FITTED", x + 14, y + 13,
                       RarityTint((Rarity)EQUIPMENT[i].rarity));
        } else {
            UiDrawText("EMPTY", x + 4, y + 13, on ? UI_TEXT : UI_DIM);
        }
    }

    const EquipSlot *e = &EQUIPMENT[f->cursor];
    UiDetail(e->slot, e->fitted ? e->fitted : "- empty -", e->desc,
               e->fitted ? (int)e->rarity : -1);
}

static void DrawMap(const MenuFrame *f)
{
    UiPageChrome("MAP", HINT_TRAVEL);

    for (int i = 0; i < DEST_COUNT; i++) {
        const int y = BODY_Y + 6 + i * 14;
        UiRow(LIST_X, y, LIST_W, DESTINATIONS[i].name, i == f->cursor, -1, 0);
    }

    const Destination *d = &DESTINATIONS[f->cursor];
    const char *sub = (d->scene == SCENE_NONE) ? "NOT BUILT YET"
                    : (d->reachable ? "REACHABLE" : "CLOSED");
    UiDetail(d->name, sub, d->desc, -1);
}

void UiMenuDraw(const UiMenu *m)
{
    if (m->depth == 0) return;

    /* Only the top frame is drawn. Stacking translucent panels would dim the
       scene twice and make the alpha look like a bug. */
    const MenuFrame *f = &m->stack[m->depth - 1];

    switch (f->screen) {
    case SCREEN_ROOT:      DrawRoot(m, f); break;
    case SCREEN_INVENTORY: DrawInventory(m, f); break;
    case SCREEN_EQUIPMENT: DrawEquipment(f); break;
    default:               DrawMap(f); break;
    }
}
