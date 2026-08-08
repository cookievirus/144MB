#include "shop.h"

#include "ui_font.h"

/* ---- layout ------------------------------------------------------------ */

#define TAB_Y      (BODY_Y - 2)
#define TAB_H      13
#define ROWS_Y     (BODY_Y + 20)
#define PRICE_X    (LIST_X + LIST_W - 4)
#define GOLD_X     (PAGE_X + PAGE_W - 14)
#define RESULT_Y   (HINT_Y - 18)

#define RESULT_SECONDS 1.6f

/* Up and down are not spelled out: the list has a highlighted row and a
   caret, so the one thing the player will try first is the one thing that
   needs no label. Left and right do, because switching tab is not something
   a list implies. */
#define HINT_SHOP "L/R TAB  SPACE TRADE  R/T/A SORT  ESC LEAVE"
UI_HINT_FITS(HINT_SHOP);

static const char *const TAB_NAMES[SHOP_TAB_COUNT] = { "BUY", "SELL" };

/* ---- list model --------------------------------------------------------

   The two tabs are different lists over the same item table: BUY walks the
   stock rows, SELL walks whatever the player is actually carrying. Both are
   computed on the fly from one flat table rather than kept as a maintained
   index, which is the same trade the inventory screen already makes: a few
   dozen comparisons on a key press against a pointer array that would sit in
   memory forever and go stale after every trade. */

/* Both tabs are flattened into the shared sort scratch, so the cursor, the
   rows and the detail pane always read one ordered list. Rebuilt per call
   rather than cached: the held counts and the stock change under it on every
   trade, and at this size the walk is cheaper than knowing when to invalidate.

   `ref` means different things per tab - a SHOP_STOCK row when buying, an
   ItemId when selling - which is exactly why the sort does not look at it. */
static int BuildRows(const UiShop *s)
{
    SortRow *rows = SortScratch();
    int n = 0;

    if (s->tab == SHOP_BUY) {
        for (int i = 0; i < STOCK_COUNT; i++) {
            const int item = SHOP_STOCK[i].item;
            rows[n].name = ITEMS[item].name;
            /* Quantity means what is for sale here, not what is in the pack:
               sorting a shop list by the player's holdings would order it by
               something the shelf knows nothing about. */
            rows[n].qty = s->stock[i];
            rows[n].rarity = ITEMS[item].rarity;
            rows[n].ref = (unsigned char)i;
            n++;
        }
    } else {
        for (int i = 0; i < ITEM_COUNT; i++) {
            if (InvHeld(i) <= 0) continue;
            rows[n].name = ITEMS[i].name;
            rows[n].qty = (unsigned short)InvHeld(i);
            rows[n].rarity = ITEMS[i].rarity;
            rows[n].ref = (unsigned char)i;
            n++;
        }
    }

    SortApply(&s->sort, rows, n);
    return n;
}

static int RowCount(const UiShop *s) { return BuildRows(s); }

/* Item id for a visible row, or -1 if the row is out of range. */
static int RowItem(const UiShop *s, int row)
{
    const int n = BuildRows(s);
    if (row < 0 || row >= n) return -1;
    const SortRow *rows = SortScratch();
    return (s->tab == SHOP_BUY) ? (int)SHOP_STOCK[rows[row].ref].item
                                : (int)rows[row].ref;
}

/* SHOP_STOCK index for a visible row on the BUY tab, or -1. */
static int RowStock(const UiShop *s, int row)
{
    if (s->tab != SHOP_BUY) return -1;
    const int n = BuildRows(s);
    if (row < 0 || row >= n) return -1;
    return (int)SortScratch()[row].ref;
}

static int RowPrice(const UiShop *s, int row)
{
    if (s->tab == SHOP_BUY) {
        const int st = RowStock(s, row);
        return (st < 0) ? 0 : InvBuyPrice(st);
    }
    const int item = RowItem(s, row);
    return (item < 0) ? 0 : InvSellPrice(item);
}

/* ---- lifecycle --------------------------------------------------------- */

void ShopRestock(UiShop *s)
{
    for (int i = 0; i < STOCK_COUNT; i++) s->stock[i] = SHOP_STOCK[i].stock;
}

void ShopInit(UiShop *s)
{
    ShopRestock(s);
    s->tab = SHOP_BUY;
    s->cursor = 0;
    s->scroll = 0;
    s->open = false;
    s->result = NULL;
    s->result_time = 0.0f;
    SortReset(&s->sort);
}

bool ShopIsOpen(const UiShop *s) { return s->open; }

void ShopOpen(UiShop *s)
{
    s->open = true;
    s->tab = SHOP_BUY;
    s->cursor = 0;
    s->scroll = 0;
    s->result = NULL;
    s->result_time = 0.0f;
}

void ShopResume(UiShop *s)
{
    s->open = true;
    s->result = NULL;
    s->result_time = 0.0f;
}

void ShopClose(UiShop *s) { s->open = false; }

void ShopSort(UiShop *s, SortMode mode)
{
    if (!s->open) return;
    SortPress(&s->sort, mode);
    /* Reordering under a fixed cursor would silently move the selection onto
       a different item, so the cursor goes back to the top of the new order. */
    s->cursor = 0;
    s->scroll = 0;
    s->result = NULL;
}

void ShopUpdate(UiShop *s, float dt)
{
    if (!s->open || s->result == NULL) return;
    s->result_time += dt;
    if (s->result_time >= RESULT_SECONDS) s->result = NULL;
}

static void Say(UiShop *s, const char *msg)
{
    s->result = msg;
    s->result_time = 0.0f;
}

/* ---- input ------------------------------------------------------------- */

static void ClampCursor(UiShop *s)
{
    const int n = RowCount(s);
    if (n <= 0) { s->cursor = 0; s->scroll = 0; return; }
    if (s->cursor >= n) s->cursor = (signed char)(n - 1);
    if (s->cursor < 0) s->cursor = 0;

    /* Keep the cursor inside the window without recentring it, matching the
       inventory list so the two scroll identically. */
    if (s->cursor < s->scroll) s->scroll = s->cursor;
    if (s->cursor >= s->scroll + LIST_ROWS) {
        s->scroll = (signed char)(s->cursor - LIST_ROWS + 1);
    }
}

static void Trade(UiShop *s)
{
    const int row = s->cursor;
    const int item = RowItem(s, row);
    if (item < 0) { Say(s, "Nothing to trade."); return; }

    const int price = RowPrice(s, row);

    if (s->tab == SHOP_BUY) {
        /* The stock index is not the row index once the list is sorted. */
        const int st = RowStock(s, row);
        if (st < 0 || s->stock[st] == 0) { Say(s, "Sold out today."); return; }
        if (price > InvGold())        { Say(s, "Not enough coin."); return; }
        if (!InvBuy(item, price))     { Say(s, "He will not sell."); return; }
        s->stock[st]--;
        Say(s, "Bought one.");
    } else {
        if (!InvSell(item, price))    { Say(s, "You hold none."); return; }
        Say(s, "Sold one.");
        /* Selling the last unit removes the row, so the cursor has to be
           pulled back or it points past the end of a list that just shrank. */
        ClampCursor(s);
    }
}

void ShopInput(UiShop *s, int dx, int dy, bool accept)
{
    if (!s->open) return;

    if (dx) {
        s->tab = (signed char)((s->tab + dx + SHOP_TAB_COUNT) % SHOP_TAB_COUNT);
        s->cursor = 0;
        s->scroll = 0;
        s->result = NULL;
    }
    if (dy) s->cursor = (signed char)(s->cursor + dy);
    ClampCursor(s);

    if (accept) Trade(s);
}

/* ---- drawing ----------------------------------------------------------- */

void ShopDraw(const UiShop *s)
{
    if (!s->open) return;

    UiPageChrome("ITEM SHOP", HINT_SHOP);

    /* Purse, on the title line where the player is already looking when the
       price list is what they are reading. */
    UiMoney(GOLD_X + 14, PAGE_Y + 7, InvGold(), UI_TEXT);
    UiDrawText(SortLabel(&s->sort), SORT_TAG_X, PAGE_Y + 7, UI_DIM);

    int tx = LIST_X;
    for (int t = 0; t < SHOP_TAB_COUNT; t++) {
        const int w = UiTextWidth(TAB_NAMES[t]) + 10;
        const bool on = (t == s->tab);
        if (on) UiPanel(tx, TAB_Y, w, TAB_H, UI_PLATE, UI_EDGE);
        UiDrawText(TAB_NAMES[t], tx + 5, TAB_Y + 3, on ? UI_TEXT : UI_DIM);
        tx += w + 3;
    }

    const int n = BuildRows(s);
    const SortRow *rows = SortScratch();

    for (int i = 0; i < LIST_ROWS; i++) {
        const int row = s->scroll + i;
        if (row >= n) break;

        const int y = ROWS_Y + i * ROW_H;
        const bool on = (row == s->cursor);
        const bool empty = (s->tab == SHOP_BUY) && (rows[row].qty == 0);

        UiRow(LIST_X, y, LIST_W, rows[row].name, on,
              empty ? -1 : (int)rows[row].rarity, ROW_MONEY_RESERVE);
        UiMoney(PRICE_X, y, RowPrice(s, row),
                empty ? UI_DIM : (on ? UI_TEXT : UI_DIM));
    }

    if (n == 0) {
        UiDrawText("Your pack is empty.", LIST_X + ROW_TEXT_X, ROWS_Y, UI_DIM);
    } else {
        const int row = s->cursor;
        const int item = RowItem(s, row);
        if (item >= 0) {
            UiDetail(ITEMS[item].name, NULL, ITEMS[item].desc,
                     (int)ITEMS[item].rarity);

            int y = DETAIL_STAT_Y;
            UiRule(DETAIL_X, y - 6, DETAIL_W, UI_EDGE);
            UiDrawText("HELD", DETAIL_X, y, UI_DIM);
            UiCount(DETAIL_X + DETAIL_W, y, InvHeld(item), UI_TEXT);

            y += 11;
            if (s->tab == SHOP_BUY) {
                const int st = RowStock(s, row);
                UiDrawText("STOCK", DETAIL_X, y, UI_DIM);
                UiCount(DETAIL_X + DETAIL_W, y,
                        (st < 0) ? 0 : (int)s->stock[st], UI_TEXT);
            } else {
                UiDrawText("PAYS", DETAIL_X, y, UI_DIM);
                UiMoney(DETAIL_X + DETAIL_W, y, RowPrice(s, row), UI_TEXT);
            }
        }
    }

    if (s->result != NULL) {
        UiDrawText(s->result, LIST_X, RESULT_Y, UI_TEXT);
    }
}
