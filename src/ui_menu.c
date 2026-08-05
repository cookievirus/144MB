#include "ui_menu.h"

#include "ui_font.h"

/* ---- layout, in virtual-screen pixels ---------------------------------- */

#define ROOT_X   8
#define ROOT_Y   8
#define ROOT_W 142
#define ROOT_H  54
#define ROOT_COLS 2
#define CELL_W   64
#define CELL_H   16
#define CELL_GAP  2
#define ROOT_PAD  8

#define PAGE_X   8
#define PAGE_Y   8
#define PAGE_W 304
#define PAGE_H 224

#define LIST_X   14
#define LIST_W  148
#define DETAIL_X 168
#define DETAIL_W 138          /* 23 columns at 6 px per glyph */
#define BODY_Y   34
#define ROW_H    11
#define LIST_ROWS 14

#define HINT_Y  219

static const char *const ROOT_LABELS[4] = { "TALK", "INVENTORY", "EQUIPMENT", "MAP" };

/* ---- helpers ----------------------------------------------------------- */

static int Wrap(int v, int n) { return (v % n + n) % n; }

static int Clamp(int v, int lo, int hi)
{
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

static int CatCount(int cat)
{
    int n = 0;
    for (int i = 0; i < ITEM_COUNT; i++) if (ITEMS[i].cat == cat) n++;
    return n;
}

/* Items are stored in one flat table and filtered on the fly rather than
   duplicated into per-category arrays: the walk is a few dozen comparisons on
   a key press, and the alternative costs pointer tables in .rodata forever. */
static const ItemDef *CatItem(int cat, int n)
{
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (ITEMS[i].cat != cat) continue;
        if (n-- == 0) return &ITEMS[i];
    }
    return NULL;
}

static void DrawRow(int x, int y, int w, const char *label, bool selected)
{
    if (selected) {
        DrawRectangle(x, y - 2, w, ROW_H, UI_SELECT);
        UiDrawText(">", x + 2, y, UI_TEXT);
    }
    UiDrawText(label, x + 10, y, selected ? UI_TEXT : UI_DIM);
}

static void DrawQty(int right, int y, unsigned short qty, bool selected)
{
    char buf[8];
    int n = 0;
    unsigned short v = qty;
    do { buf[n++] = (char)('0' + v % 10); v = (unsigned short)(v / 10); } while (v && n < 6);
    char out[8];
    for (int i = 0; i < n; i++) out[i] = buf[n - 1 - i];
    out[n] = '\0';
    UiDrawText(out, right - n * FONT_CELL_W, y, selected ? UI_TEXT : UI_DIM);
}

static void DrawPageChrome(const char *title)
{
    UiPanel(PAGE_X, PAGE_Y, PAGE_W, PAGE_H, UI_FILL, UI_EDGE);
    UiDrawText(title, LIST_X, PAGE_Y + 7, UI_TEXT);
    UiRule(LIST_X, PAGE_Y + 19, PAGE_W - 12, UI_EDGE);
    UiRule(LIST_X, HINT_Y - 5, PAGE_W - 12, UI_EDGE);
    UiDrawText("ARROWS MOVE   SPACE/ENTER SELECT   ESC BACK", LIST_X, HINT_Y, UI_DIM);
}

static void DrawDetail(const char *heading, const char *sub, const char *body)
{
    DrawRectangle(DETAIL_X - 4, BODY_Y - 2, DETAIL_W + 8, HINT_Y - BODY_Y - 6, UI_SHADE);
    int y = BODY_Y + 4;
    UiDrawText(heading, DETAIL_X, y, UI_TEXT);
    y += 12;
    if (sub != NULL) {
        UiDrawText(sub, DETAIL_X, y, UI_DIM);
        y += 12;
    }
    UiRule(DETAIL_X, y, DETAIL_W, UI_EDGE);
    UiDrawText(body, DETAIL_X, y + 7, UI_TEXT);
}

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

void UiMenuInit(UiMenu *m) { m->depth = 0; }
bool UiMenuIsOpen(const UiMenu *m) { return m->depth > 0; }
void UiMenuClose(UiMenu *m) { m->depth = 0; }

void UiMenuOpen(UiMenu *m)
{
    m->depth = 0;
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
        int col = f->cursor % ROOT_COLS;
        int row = f->cursor / ROOT_COLS;
        if (dx) col = Wrap(col + dx, ROOT_COLS);
        if (dy) row = Wrap(row + dy, 2);
        f->cursor = (signed char)(row * ROOT_COLS + col);

        if (accept) {
            switch (f->cursor) {
            case 0: UiMenuClose(m); return MENU_CMD_TALK;
            case 1: Push(m, SCREEN_INVENTORY); break;
            case 2: Push(m, SCREEN_EQUIPMENT); break;
            default: Push(m, SCREEN_MAP); break;
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
        break;
    }

    (void)accept;
    return MENU_CMD_NONE;
}

/* ---- drawing ----------------------------------------------------------- */

static void DrawRoot(const MenuFrame *f)
{
    UiPanel(ROOT_X, ROOT_Y, ROOT_W, ROOT_H, UI_FILL, UI_EDGE);

    for (int i = 0; i < 4; i++) {
        const int col = i % ROOT_COLS;
        const int row = i / ROOT_COLS;
        const int x = ROOT_X + ROOT_PAD + col * (CELL_W + CELL_GAP);
        const int y = ROOT_Y + ROOT_PAD + row * (CELL_H + CELL_GAP);
        const bool on = (i == f->cursor);

        if (on) UiPanel(x, y, CELL_W, CELL_H, UI_SELECT, UI_EDGE);
        UiDrawText(ROOT_LABELS[i], x + 5, y + 4, on ? UI_TEXT : UI_DIM);
    }
}

static void DrawInventory(const MenuFrame *f)
{
    DrawPageChrome("INVENTORY");

    int tx = LIST_X;
    for (int c = 0; c < CAT_COUNT; c++) {
        const int w = UiTextWidth(CAT_NAMES[c]) + 10;
        const bool on = (c == f->tab);
        if (on) UiPanel(tx, BODY_Y - 2, w, 13, UI_PLATE, UI_EDGE);
        UiDrawText(CAT_NAMES[c], tx + 5, BODY_Y + 1, on ? UI_TEXT : UI_DIM);
        tx += w + 3;
    }

    const int n = CatCount(f->tab);
    for (int i = 0; i < LIST_ROWS; i++) {
        const int idx = f->scroll + i;
        if (idx >= n) break;
        const ItemDef *it = CatItem(f->tab, idx);
        const int y = BODY_Y + 20 + i * ROW_H;
        const bool on = (idx == f->cursor);
        DrawRow(LIST_X, y, LIST_W, it->name, on);
        DrawQty(LIST_X + LIST_W - 4, y, it->qty, on);
    }

    if (n > 0) {
        const ItemDef *it = CatItem(f->tab, f->cursor);
        char held[20] = "HELD: ";
        int k = 6;
        unsigned short v = it->qty;
        char d[6];
        int c = 0;
        do { d[c++] = (char)('0' + v % 10); v = (unsigned short)(v / 10); } while (v && c < 5);
        while (c > 0) held[k++] = d[--c];
        held[k] = '\0';
        DrawDetail(it->name, held, it->desc);
    }
}

static void DrawEquipment(const MenuFrame *f)
{
    DrawPageChrome("EQUIPMENT");

    for (int i = 0; i < EQUIP_COUNT; i++) {
        const int col = i % 2;
        const int row = i / 2;
        const int x = LIST_X + col * 76;
        const int y = BODY_Y + 4 + row * 28;
        const bool on = (i == f->cursor);

        UiPanel(x, y, 74, 24, on ? UI_SELECT : UI_SHADE, UI_EDGE);
        UiDrawText(EQUIPMENT[i].slot, x + 4, y + 4, on ? UI_TEXT : UI_DIM);
        UiDrawText(EQUIPMENT[i].fitted ? "FITTED" : "EMPTY",
                   x + 4, y + 13, on ? UI_TEXT : UI_DIM);
    }

    const EquipSlot *e = &EQUIPMENT[f->cursor];
    DrawDetail(e->slot, e->fitted ? e->fitted : "- empty -", e->desc);
}

static void DrawMap(const MenuFrame *f)
{
    DrawPageChrome("MAP");

    for (int i = 0; i < DEST_COUNT; i++) {
        const int y = BODY_Y + 6 + i * 14;
        DrawRow(LIST_X, y, LIST_W, DESTINATIONS[i].name, i == f->cursor);
    }

    const Destination *d = &DESTINATIONS[f->cursor];
    DrawDetail(d->name, d->reachable ? "REACHABLE" : "CLOSED", d->desc);
    UiDrawText("TRAVEL NOT IMPLEMENTED", LIST_X, HINT_Y - 20, UI_DIM);
}

void UiMenuDraw(const UiMenu *m)
{
    if (m->depth == 0) return;

    /* Only the top frame is drawn. Stacking translucent panels would dim the
       scene twice and make the alpha look like a bug. */
    const MenuFrame *f = &m->stack[m->depth - 1];

    switch (f->screen) {
    case SCREEN_ROOT:      DrawRoot(f); break;
    case SCREEN_INVENTORY: DrawInventory(f); break;
    case SCREEN_EQUIPMENT: DrawEquipment(f); break;
    default:               DrawMap(f); break;
    }
}
