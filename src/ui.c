#include "ui.h"

#include "ui_font.h"

const Color UI_FILL   = {  14,  18,  34, 208 };
const Color UI_EDGE   = { 152, 164, 190, 255 };
const Color UI_PLATE  = {  26,  42,  80, 232 };
const Color UI_SELECT = {  58,  86, 142, 235 };
const Color UI_TEXT   = { 240, 238, 228, 255 };
const Color UI_DIM    = { 132, 140, 160, 255 };
const Color UI_SHADE  = {   8,  10,  18, 190 };

void UiPanel(int x, int y, int w, int h, Color fill, Color edge)
{
    DrawRectangle(x + 1, y, w - 2, h, fill);
    DrawRectangle(x, y + 1, 1, h - 2, fill);
    DrawRectangle(x + w - 1, y + 1, 1, h - 2, fill);

    DrawRectangle(x + 1, y, w - 2, 1, edge);
    DrawRectangle(x + 1, y + h - 1, w - 2, 1, edge);
    DrawRectangle(x, y + 1, 1, h - 2, edge);
    DrawRectangle(x + w - 1, y + 1, 1, h - 2, edge);
}

void UiRule(int x, int y, int w, Color c)
{
    DrawRectangle(x, y, w, 1, c);
}

/* ---- full-page screen furniture ---------------------------------------- */

void UiPageChrome(const char *title, const char *hint)
{
    UiPanel(PAGE_X, PAGE_Y, PAGE_W, PAGE_H, UI_FILL, UI_EDGE);
    UiDrawText(title, LIST_X, PAGE_Y + 7, UI_TEXT);
    UiRule(LIST_X, PAGE_Y + 19, PAGE_W - 12, UI_EDGE);
    UiRule(LIST_X, HINT_Y - 5, PAGE_W - 12, UI_EDGE);
    UiDrawText(hint, LIST_X, HINT_Y, UI_DIM);
}

void UiRow(int x, int y, int w, const char *label, bool selected, int rarity,
           int reserve)
{
    if (selected) {
        DrawRectangle(x, y - 2, w, ROW_H, UI_SELECT);
        UiDrawText(">", x + ROW_MARK_X, y, UI_TEXT);
    }

    const Color tint = (rarity >= 0) ? RarityTint((Rarity)rarity)
                                     : (selected ? UI_TEXT : UI_DIM);
    if (rarity >= 0) RarityBall((Rarity)rarity, x + ROW_BALL_X, y - 1);

    const int room = w - ROW_TEXT_X - reserve;
    const int max = room / FONT_CELL_W;
    int len = 0;
    while (label[len] != '\0') len++;

    if (max <= 0) return;
    if (len <= max) {
        UiDrawText(label, x + ROW_TEXT_X, y, tint);
        return;
    }

    /* Cut one short and mark it, so a clipped name never looks like a real
       one that happens to end oddly. */
    UiDrawTextN(label, max - 1, x + ROW_TEXT_X, y, tint);
    UiDrawText(".", x + ROW_TEXT_X + (max - 1) * FONT_CELL_W, y, tint);
}

void UiNumber(int right, int y, int value, Color tint)
{
    char digits[8];
    char out[9];
    int n = 0;
    int k = 0;
    unsigned int v;

    if (value < 0) { out[k++] = '-'; v = (unsigned int)(-value); }
    else           { v = (unsigned int)value; }

    do { digits[n++] = (char)('0' + v % 10u); v /= 10u; } while (v && n < 7);
    while (n > 0) out[k++] = digits[--n];
    out[k] = '\0';

    UiDrawText(out, right - k * FONT_CELL_W, y, tint);
}

void UiMoney(int right, int y, int value, Color tint)
{
    UiDrawText("G", right - FONT_CELL_W, y, tint);
    UiNumber(right - FONT_CELL_W * 2, y, value, tint);
}

void UiCount(int right, int y, int value, Color tint)
{
    UiDrawText("EA", right - FONT_CELL_W * 2, y, tint);
    UiNumber(right - FONT_CELL_W * 3, y, value, tint);
}

void UiDetail(const char *heading, const char *sub, const char *body,
              int rarity)
{
    DrawRectangle(DETAIL_X - 4, BODY_Y - 2, DETAIL_W + 8, HINT_Y - BODY_Y - 6,
                  UI_SHADE);
    int y = BODY_Y + 4;
    UiDrawText(heading, DETAIL_X, y, UI_TEXT);
    y += 12;
    if (rarity >= 0) {
        RarityBall((Rarity)rarity, DETAIL_X, y - 1);
        UiDrawText(RarityName((Rarity)rarity), DETAIL_X + 10, y,
                   RarityTint((Rarity)rarity));
        y += 12;
    }
    if (sub != NULL) {
        UiDrawText(sub, DETAIL_X, y, UI_DIM);
        y += 12;
    }
    UiRule(DETAIL_X, y, DETAIL_W, UI_EDGE);
    UiDrawText(body, DETAIL_X, y + 7, UI_TEXT);
}
