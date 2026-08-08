#include "forge.h"

#include "ui_font.h"

/* ---- layout ------------------------------------------------------------

   Every macro here is prefixed F_ and every static function is prefixed
   Forge. title.c and ui_prompt.c both defined TITLE_Y in 1.4 and the unity
   build silently kept the second one; the compiler only warned because the
   values happened to differ.

   1.5 hit the same hazard one level up. `static` scopes a function to its
   translation unit, and in a unity build there is exactly one of those - so
   forge.c's BuildRows, ClampCursor and Say collided head-on with shop.c's,
   which are the same three helpers doing the same three jobs for the other
   list screen. That one is at least a hard error rather than a silent
   substitution, because the signatures differ. Two screens whose helpers
   happened to take the same type would not have been so lucky. */

#define F_TAB_Y      (BODY_Y - 2)
#define F_TAB_H      13
#define F_ROWS_Y     (BODY_Y + 20)
#define F_HELD_X     (LIST_X + LIST_W - 4)
#define F_RESULT_Y   (HINT_Y - 18)

#define F_RESULT_SECONDS 1.6f

/* Ingredient readout in the detail pane. Three slots at ROW_H, sitting above
   the status line rather than at DETAIL_STAT_Y: the hand-wrapped description
   runs to roughly y=125 at five lines, and the "not enough" line needs a row
   of its own underneath. */
#define F_MAT_Y      167
#define F_STATUS_Y   (F_MAT_Y + 3 * ROW_H)

/* Ingredient name column. The count column is "40/40" at worst, which is
   5 glyphs plus a gap. */
#define F_COUNT_W    32
#define F_NAME_CHARS ((DETAIL_W - F_COUNT_W) / FONT_CELL_W)   /* 17 */

#define F_HINT_CRAFT "L/R SHELF  SPACE FORGE  R/T/A SORT  ESC BACK"
UI_HINT_FITS(F_HINT_CRAFT);

#define F_HINT_BOOK "R/T/A SORT  ESC BACK"
UI_HINT_FITS(F_HINT_BOOK);

/* ---- list model --------------------------------------------------------

   One builder for both modes. FORGE walks the recipes on the selected shelf;
   BLUEPRINTS walks every recipe the player knows, which is why it ignores the
   tab rather than having a third tab called ALL.

   `ref` is the recipe index and the sort never looks at it - the same
   indirection the shop needed once its rows could be reordered. Reading
   RECIPES[row] after a sort would forge whatever recipe happened to land in
   that slot, which is precisely the bug that shipped in the shop's BUY tab in
   1.2 and was only caught when sorting arrived. */
static int ForgeBuildRows(const UiForge *f)
{
    SortRow *rows = SortScratch();
    int n = 0;

    for (int i = 0; i < RECIPE_COUNT; i++) {
        if (!InvKnows(i)) continue;
        if (f->mode == FORGE_MODE_CRAFT && RECIPES[i].cat != f->tab) continue;

        const int out = RECIPES[i].out;
        rows[n].name = ITEMS[out].name;
        /* Quantity is what the player is holding of the thing this recipe
           makes, not what it costs. Sorting by "how many have I already got"
           is the question a smith deciding what to make next actually asks. */
        rows[n].qty = (unsigned short)InvHeld(out);
        rows[n].rarity = ITEMS[out].rarity;
        rows[n].ref = (unsigned char)i;
        n++;
    }

    SortApply(&f->sort, rows, n);
    return n;
}

/* Recipe index for a visible row, or -1 if the row is out of range. */
static int ForgeRowRecipe(const UiForge *f, int row)
{
    const int n = ForgeBuildRows(f);
    if (row < 0 || row >= n) return -1;
    return (int)SortScratch()[row].ref;
}

/* ---- lifecycle --------------------------------------------------------- */

void ForgeInit(UiForge *f)
{
    f->mode = FORGE_MODE_CRAFT;
    f->tab = FORGE_WEAPON;
    f->cursor = 0;
    f->scroll = 0;
    f->open = false;
    f->result = NULL;
    f->result_time = 0.0f;
    SortReset(&f->sort);
}

bool ForgeIsOpen(const UiForge *f) { return f->open; }

void ForgeOpen(UiForge *f, ForgeMode mode)
{
    f->open = true;
    f->mode = (signed char)mode;
    f->tab = FORGE_WEAPON;
    f->cursor = 0;
    f->scroll = 0;
    f->result = NULL;
    f->result_time = 0.0f;
}

void ForgeClose(UiForge *f) { f->open = false; }

void ForgeResume(UiForge *f, const char *result)
{
    f->open = true;
    f->result = result;
    f->result_time = 0.0f;
}

void ForgeSort(UiForge *f, SortMode mode)
{
    if (!f->open) return;
    SortPress(&f->sort, mode);
    f->cursor = 0;
    f->scroll = 0;
    f->result = NULL;
}

void ForgeUpdate(UiForge *f, float dt)
{
    if (!f->open || f->result == NULL) return;
    f->result_time += dt;
    if (f->result_time >= F_RESULT_SECONDS) f->result = NULL;
}

static void ForgeSay(UiForge *f, const char *msg)
{
    f->result = msg;
    f->result_time = 0.0f;
}

/* ---- input ------------------------------------------------------------- */

static void ForgeClampCursor(UiForge *f)
{
    const int n = ForgeBuildRows(f);
    if (n <= 0) { f->cursor = 0; f->scroll = 0; return; }
    if (f->cursor >= n) f->cursor = (signed char)(n - 1);
    if (f->cursor < 0) f->cursor = 0;

    if (f->cursor < f->scroll) f->scroll = f->cursor;
    if (f->cursor >= f->scroll + LIST_ROWS) {
        f->scroll = (signed char)(f->cursor - LIST_ROWS + 1);
    }
}

/* The two refusals stay here because they are facts about the list - there is
   no row, or the shelf cannot pay - and the player should be told at the row
   they are looking at rather than by a minigame that opens and immediately
   closes. Everything past that point is the scene's. */
static ForgeCommand ForgeCraft(UiForge *f)
{
    const int recipe = ForgeRowRecipe(f, f->cursor);
    if (recipe < 0) { ForgeSay(f, "Nothing to forge."); return FORGE_CMD_NONE; }

    if (!InvCanForge(recipe)) {
        ForgeSay(f, "Not enough materials.");
        return FORGE_CMD_NONE;
    }
    return (ForgeCommand)(FORGE_CMD_BEGIN_BASE + recipe);
}

ForgeCommand ForgeInput(UiForge *f, int dx, int dy, bool accept)
{
    if (!f->open) return FORGE_CMD_NONE;

    /* The tabs are the FORGE menu's two shelves. BLUEPRINTS shows everything
       at once, so left and right have nothing to switch between and are
       ignored rather than being given a meaning they do not have. */
    if (dx && f->mode == FORGE_MODE_CRAFT) {
        f->tab = (signed char)((f->tab + dx + FORGE_CAT_COUNT) % FORGE_CAT_COUNT);
        f->cursor = 0;
        f->scroll = 0;
        f->result = NULL;
    }
    if (dy) f->cursor = (signed char)(f->cursor + dy);
    ForgeClampCursor(f);

    /* Blueprints are for reading. Crafting happens at the forge or not at
       all, so the accept key here does nothing rather than doing the same
       thing from two places. */
    if (accept && f->mode == FORGE_MODE_CRAFT) return ForgeCraft(f);
    return FORGE_CMD_NONE;
}

/* ---- drawing ----------------------------------------------------------- */

/* "Iron Ore              34/40". The held count carries the colour: red when
   it is short, because that is the one number the player is looking for. */
static void ForgeDrawMat(int y, const RecipeMat *m)
{
    const int have = InvHeld(m->item);
    const bool enough = (have >= (int)m->qty);

    UiDrawTextN(ITEMS[m->item].name, F_NAME_CHARS, DETAIL_X, y, UI_DIM);
    UiNumber(DETAIL_X + DETAIL_W - 14, y, have, enough ? UI_TEXT : RarityTint(RARITY_JUNK));
    UiDrawText("/", DETAIL_X + DETAIL_W - 13, y, UI_DIM);
    UiNumber(DETAIL_X + DETAIL_W, y, (int)m->qty, UI_DIM);
}

static void ForgeDrawDetail(int recipe)
{
    const RecipeDef *r = &RECIPES[recipe];
    const int out = r->out;

    UiDetail(ITEMS[out].name, FORGE_CAT_ONE[r->cat], ITEMS[out].desc,
             (int)ITEMS[out].rarity);

    UiRule(DETAIL_X, F_MAT_Y - 6, DETAIL_W, UI_EDGE);

    int y = F_MAT_Y;
    for (int i = 0; i < RECIPE_SLOTS; i++) {
        if (r->mat[i].item == RECIPE_NONE) continue;
        ForgeDrawMat(y, &r->mat[i]);
        y += ROW_H;
    }

    /* Named for the shelf it is short of rather than a bare "unavailable":
       the player's next move is to go and buy the thing, and the message that
       does not say which thing sends them back into the menu to find out. */
    if (!InvCanForge(recipe)) {
        UiDrawText("SHORT ON MATERIALS", DETAIL_X, F_STATUS_Y,
                   RarityTint(RARITY_JUNK));
    }
}

void ForgeDraw(const UiForge *f)
{
    if (!f->open) return;

    const bool craft = (f->mode == FORGE_MODE_CRAFT);

    UiPageChrome(craft ? "FORGE" : "BLUEPRINTS",
                 craft ? F_HINT_CRAFT : F_HINT_BOOK);
    UiDrawText(SortLabel(&f->sort), SORT_TAG_X, PAGE_Y + 7, UI_DIM);

    if (craft) {
        int tx = LIST_X;
        for (int t = 0; t < FORGE_CAT_COUNT; t++) {
            const int w = UiTextWidth(FORGE_CAT_NAMES[t]) + 10;
            const bool on = (t == f->tab);
            if (on) UiPanel(tx, F_TAB_Y, w, F_TAB_H, UI_PLATE, UI_EDGE);
            UiDrawText(FORGE_CAT_NAMES[t], tx + 5, F_TAB_Y + 3,
                       on ? UI_TEXT : UI_DIM);
            tx += w + 3;
        }
    } else {
        UiDrawText("EVERY RECIPE YOU KNOW", LIST_X, F_TAB_Y + 3, UI_DIM);
    }

    const int n = ForgeBuildRows(f);
    const SortRow *rows = SortScratch();

    for (int i = 0; i < LIST_ROWS; i++) {
        const int row = f->scroll + i;
        if (row >= n) break;

        const int y = F_ROWS_Y + i * ROW_H;
        const bool on = (row == f->cursor);
        /* A recipe the shelf cannot pay for loses its quality ball and its
           colour, which is the same disabled state the shop uses for a row
           that has sold out. The row keeps its place in the list. */
        const bool ready = InvCanForge((int)rows[row].ref);

        UiRow(LIST_X, y, LIST_W, rows[row].name, on,
              ready ? (int)rows[row].rarity : -1, ROW_COUNT_RESERVE);
        UiCount(F_HELD_X, y, (int)rows[row].qty,
                ready ? (on ? UI_TEXT : UI_DIM) : UI_DIM);
    }

    if (n == 0) {
        /* Only reachable in BLUEPRINTS once recipes stop being known from the
           start, and on a FORGE shelf whose recipes have not been taught yet.
           Both are real states, so both get a line rather than an empty box. */
        UiDrawText(craft ? "Nothing on this shelf yet."
                         : "You have learned no recipes.",
                   LIST_X + ROW_TEXT_X, F_ROWS_Y, UI_DIM);
    } else {
        const int recipe = ForgeRowRecipe(f, f->cursor);
        if (recipe >= 0) ForgeDrawDetail(recipe);
    }

    if (f->result != NULL) UiDrawText(f->result, LIST_X, F_RESULT_Y, UI_TEXT);
}
