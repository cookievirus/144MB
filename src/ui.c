#include "ui.h"

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
