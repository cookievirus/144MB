#include "qte.h"

#include "ui_font.h"

/* Every macro is prefixed Q_ and every static Qte, for the reason forge.c
   spells out at length: this is a unity build, so file scope is not scope. */

/* ---- tuning ------------------------------------------------------------

   The sequence length comes from the tier of the thing being made rather than
   from a per-recipe field. A Rare staff is seven steps and a Common dagger is
   five, and retuning the whole curve is one constant instead of a pass over
   the table. The cost is that difficulty cannot be authored per recipe; when
   a boss commission wants a fixed eleven-step sequence, that is the point at
   which it earns a byte in RecipeDef. */
#define Q_BASE_STEPS   4
#define Q_WINDOW       0.85f   /* seconds for the first step   */
#define Q_TIGHTEN      0.045f  /* shaved off each step after it */
#define Q_MIN_WINDOW   0.45f
#define Q_FLASH        0.12f
#define Q_VERDICT      1.10f   /* how long the result stays up  */

/* Misses tolerated before the heat is ruined. A third of the sequence, so a
   longer sequence is not punished twice for being longer. Integer division
   floors it, which means a five-step run allows exactly one miss. */
#define Q_TOLERANCE(steps) ((steps) / 3)

/* ---- layout ------------------------------------------------------------ */

#define Q_BTN_W   24
#define Q_BTN_H   20
#define Q_GAP      4
#define Q_ROW_Y  172
#define Q_BAR_Y  204
#define Q_BAR_W  200
#define Q_TEXT_Y 214

static const Color Q_DIM   = {  74,  58,  44, 255 };   /* not yet, and done  */
static const Color Q_NEXT  = { 126, 100,  66, 255 };   /* the one after this */
static const Color Q_GLOW  = { 246, 170,  44, 255 };   /* live               */
static const Color Q_HOT   = { 255, 214, 130, 255 };   /* live glyph         */
static const Color Q_MISS  = { 152,  52,  40, 255 };   /* a step gone wrong  */
static const Color Q_WASH  = {   8,   6,   6, 170 };   /* the room, dimmed   */

/* ---- sequence ---------------------------------------------------------- */

static unsigned int g_qte_seed = 0x1F2Eu;

void QteSeed(unsigned int seed) { g_qte_seed = seed ? seed : 1u; }

static unsigned int QteRand(void)
{
    unsigned int x = g_qte_seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_qte_seed = x;
    return x;
}

void QteInit(UiQte *q)
{
    q->open = false;
    q->done = false;
    q->steps = 0;
    q->at = 0;
    q->misses = 0;
    q->missed = 0;
    q->recipe = -1;
    q->timer = 0.0f;
    q->window = Q_WINDOW;
    q->flash = 0.0f;
    q->flash_kind = 0;
    q->done_time = 0.0f;
    q->grade = QTE_PLAIN;
}

bool QteIsOpen(const UiQte *q) { return q->open; }

static float QteWindowFor(int step)
{
    const float w = Q_WINDOW - Q_TIGHTEN * (float)step;
    return (w < Q_MIN_WINDOW) ? Q_MIN_WINDOW : w;
}

void QteBegin(UiQte *q, int recipe)
{
    QteInit(q);
    q->open = true;
    q->recipe = (signed char)recipe;

    int n = Q_BASE_STEPS + (int)ITEMS[RECIPES[recipe].out].rarity;
    if (n > QTE_MAX_STEPS) n = QTE_MAX_STEPS;
    if (n < 2) n = 2;
    q->steps = (unsigned char)n;

    /* A step never repeats the one before it. Two identical prompts in a row
       are indistinguishable from an input that did not register, and the
       player blames the game rather than their thumb. */
    unsigned char prev = QTE_MAX_STEPS;   /* impossible key */
    for (int i = 0; i < n; i++) {
        unsigned char k;
        do { k = (unsigned char)(QteRand() % QK_KEY_COUNT); } while (k == prev);
        q->step[i] = k;
        prev = k;
    }

    q->window = QteWindowFor(0);
    q->timer = q->window;
}

/* ---- play -------------------------------------------------------------- */

static void QteAdvance(UiQte *q, bool hit)
{
    if (!hit) {
        q->missed |= (unsigned short)(1u << q->at);
        q->misses++;
    }
    q->flash = Q_FLASH;
    q->flash_kind = hit ? 1 : -1;

    q->at++;
    if (q->at >= q->steps) {
        q->done = true;
        q->done_time = 0.0f;
        q->grade = (q->misses == 0)                    ? QTE_FINE
                 : (q->misses <= Q_TOLERANCE(q->steps)) ? QTE_PLAIN
                                                        : QTE_RUINED;
        return;
    }
    q->window = QteWindowFor(q->at);
    q->timer = q->window;
}

void QteKeyPress(UiQte *q, QteKey key)
{
    if (!q->open || q->done) return;
    QteAdvance(q, key == (QteKey)q->step[q->at]);
}

void QteUpdate(UiQte *q, float dt)
{
    if (!q->open) return;

    if (q->flash > 0.0f) q->flash -= dt;

    if (q->done) { q->done_time += dt; return; }

    q->timer -= dt;
    /* A window that runs out is a miss and advances, so a player who stops
       pressing keys still reaches a verdict. There is no abandon key because
       there is nothing to abandon into. */
    if (q->timer <= 0.0f) QteAdvance(q, false);
}

bool QteFinished(const UiQte *q)
{
    return q->open && q->done && q->done_time >= Q_VERDICT;
}

QteGrade QteResult(const UiQte *q) { return (QteGrade)q->grade; }

void QteClose(UiQte *q) { q->open = false; }

int QteOutput(int recipe, QteGrade grade)
{
    if (recipe < 0 || recipe >= RECIPE_COUNT) return -1;
    if (grade == QTE_RUINED) return -1;

    const RecipeDef *r = &RECIPES[recipe];
    if (grade == QTE_FINE && r->fine != RECIPE_NONE) return (int)r->fine;
    return (int)r->out;
}

/* ---- drawing -----------------------------------------------------------

   Arrows are built from rectangles like every other widget in the game. The
   font is ASCII and has no arrow glyphs, and adding four would cost a font
   rebuild plus four cells to draw shapes that are sixteen rectangles here.
   The SPACE key is a flat bar rather than the word, per the brief: a text
   label at this size is four pixels tall and reads as noise. */
static void QteChevron(int cx, int cy, int dir, Color c)
{
    for (int i = 0; i < 4; i++) {
        const int w = 2 + i * 2;
        switch (dir) {
        case QK_UP:    DrawRectangle(cx - w / 2, cy - 4 + i * 2, w, 2, c); break;
        case QK_DOWN:  DrawRectangle(cx - w / 2, cy + 2 - i * 2, w, 2, c); break;
        case QK_LEFT:  DrawRectangle(cx - 4 + i * 2, cy - w / 2, 2, w, c); break;
        default:       DrawRectangle(cx + 2 - i * 2, cy - w / 2, 2, w, c); break;
        }
    }
}

static void QteGlyph(int key, int cx, int cy, Color c)
{
    if (key == QK_SPACE) DrawRectangle(cx - 7, cy - 2, 14, 4, c);
    else                 QteChevron(cx, cy, key, c);
}

void QteDraw(const UiQte *q)
{
    if (!q->open) return;

    /* Modal, so it washes the frame - the same rule UiPrompt follows. A
       minigame the player can mistake for scenery is one they will lose. */
    DrawRectangle(0, 0, VSCREEN_W, VSCREEN_H, Q_WASH);

    const int row_w = q->steps * Q_BTN_W + (q->steps - 1) * Q_GAP;
    const int x0 = (VSCREEN_W - row_w) / 2;

    for (int i = 0; i < q->steps; i++) {
        const int x = x0 + i * (Q_BTN_W + Q_GAP);
        const int cx = x + Q_BTN_W / 2;
        const int cy = Q_ROW_Y + Q_BTN_H / 2;

        const bool live = (i == q->at) && !q->done;
        const bool past = (i < q->at) || q->done;
        const bool bad = (q->missed >> i) & 1u;

        Color edge = Q_DIM, glyph = Q_DIM;
        if (past)        { edge = bad ? Q_MISS : Q_DIM; glyph = bad ? Q_MISS : Q_DIM; }
        else if (live)   { edge = Q_GLOW; glyph = Q_HOT; }
        else if (i == q->at + 1) { edge = Q_NEXT; glyph = Q_NEXT; }

        if (live) {
            UiPanel(x, Q_ROW_Y, Q_BTN_W, Q_BTN_H, UI_PLATE, edge);
            /* The window is drawn as a bar that eats itself from both ends
               under the live key. A number would be exact and useless; what
               the player needs is "now" getting narrower. */
            const float t = (q->window > 0.0f) ? (q->timer / q->window) : 0.0f;
            const int w = (int)((float)(Q_BTN_W - 4) * (t < 0.0f ? 0.0f : t));
            DrawRectangle(cx - w / 2, Q_ROW_Y + Q_BTN_H + 2, w, 1, Q_GLOW);
        } else {
            UiPanel(x, Q_ROW_Y, Q_BTN_W, Q_BTN_H, UI_SHADE, edge);
        }

        QteGlyph(q->step[i], cx, cy, glyph);

        /* Separator, so the row reads as a sequence rather than a set. */
        if (i + 1 < q->steps) {
            DrawRectangle(x + Q_BTN_W + 1, cy - 1, 2, 2,
                          (i < q->at) ? Q_DIM : Q_NEXT);
        }
    }

    /* The forging line. One pixel tall on purpose: it is the only thing on
       screen competing with the key row, and it should lose. */
    const int bx = (VSCREEN_W - Q_BAR_W) / 2;
    DrawRectangle(bx, Q_BAR_Y, Q_BAR_W, 1, Q_DIM);
    DrawRectangle(bx, Q_BAR_Y, (Q_BAR_W * q->at) / q->steps, 1, Q_GLOW);

    if (q->done) {
        static const char *const VERDICT[3] = { "FINE WORK", "IT WILL DO", "RUINED" };
        const char *v = VERDICT[q->grade];
        UiDrawText(v, (VSCREEN_W - UiTextWidth(v)) / 2, Q_TEXT_Y,
                   (q->grade == QTE_RUINED) ? Q_MISS : Q_GLOW);
    }
}
