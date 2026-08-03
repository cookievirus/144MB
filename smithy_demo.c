/* ============================================================================
 * smithy_demo.c — IRON & INVESTMENT : blacksmith interior tilemap demo
 *
 * Pure C99 + Raylib (static). Single translation unit (unity-build friendly).
 *   - 21x14 tilemap @16px from indexed atlas (assets baked at build time)
 *   - 4-direction walking character (mirror-step + 2-pose side cycle)
 *   - O(1) bitmask collision, feet-box AABB
 *   - 320x240 internal render target, integer pillarbox, F11 fullscreen
 *
 * Build (Linux):   cc -std=c99 -Os -flto smithy_demo.c -o smithy -lraylib -lm
 * Build (Windows): cc -std=c99 -Os -flto smithy_demo.c -o smithy.exe ^
 *                     -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows -s
 * Controls: WASD / arrows = walk, C = collision debug, F11 = fullscreen
 * ==========================================================================*/
#include "raylib.h"
#include "assets_smithy.h"
#include "map_smithy.h"

#define GAME_W    320
#define GAME_H    240
#define TILE      16
#define MAP_PX_W  (SMITHY_W * TILE)   /* 336 */
#define MAP_PX_H  (SMITHY_H * TILE)   /* 224 */

#define PLR_SPEED 56.0f               /* px/s */
#define FOOT_W    10
#define FOOT_H    5

typedef enum { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP } Dir;

typedef struct {
    float x, y;        /* feet center (bottom of sprite) */
    Dir   dir;
    bool  moving;
    float anim_t;
} Player;

/* ---------- 4bpp indexed -> RGBA texture (boot-time unpack) -------------
 * Trade-off: costs ~140 KB of transient RAM during load and a few ms of CPU,
 * but halves the on-disk cost vs 8bpp. RAM is free here; bytes are not.      */
static inline unsigned char nib(const unsigned char *d, int i)
{
    return (d[i >> 1] >> (((i & 1) ^ 1) << 2)) & 0x0F;
}

static Texture2D tex_from_4bpp(const unsigned char *src, int w, int h,
                               int cell_w, int cell_h, int cells, int cols)
{
    /* src is cell-major (cells x cell_h x cell_w); dst is a cols-wide sheet */
    Image img = GenImageColor(w, h, BLANK);
    Color *dst = (Color *)img.data;

    for (int c = 0; c < cells; c++) {
        int ox = (c % cols) * cell_w;
        int oy = (c / cols) * cell_h;
        for (int y = 0; y < cell_h; y++)
            for (int x = 0; x < cell_w; x++) {
                unsigned char p = nib(src, (c*cell_h + y)*cell_w + x);
                dst[(oy + y)*w + ox + x] = (Color){
                    pal_rgb[p*3], pal_rgb[p*3+1], pal_rgb[p*3+2],
                    (unsigned char)(p ? 255 : 0) };
            }
    }
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    return t;
}

/* ---------- collision -------------------------------------------------- */
static bool solid_px(float px, float py)
{
    if (px < 0 || py < 0 || px >= MAP_PX_W || py >= MAP_PX_H) return true;
    return SMITHY_SOLID((int)px / TILE, (int)py / TILE);
}

static bool foot_box_free(float x, float y)
{
    float l = x - FOOT_W/2, r = x + FOOT_W/2 - 1;
    float t = y - FOOT_H,   b = y - 1;
    return !(solid_px(l,t) || solid_px(r,t) || solid_px(l,b) || solid_px(r,b));
}

/* ---------- map draw: per-frame from indices (mirrors engine map_draw) -- */
static void map_draw(Texture2D atlas, int ox, int oy)
{
    for (int ty = 0; ty < SMITHY_H; ty++)
        for (int tx = 0; tx < SMITHY_W; tx++) {
            int id = smithy_bg[ty*SMITHY_W + tx];
            Rectangle src = { (float)(id % ATLAS_COLS) * TILE,
                              (float)(id / ATLAS_COLS) * TILE, TILE, TILE };
            DrawTextureRec(atlas, src,
                (Vector2){ (float)(tx*TILE + ox), (float)(ty*TILE + oy) }, WHITE);
        }
}

/* ---------- integer pillarbox (same as engine pillarbox_dst) ------------ */
static Rectangle pillarbox_dst(void)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int s  = sw / GAME_W < sh / GAME_H ? sw / GAME_W : sh / GAME_H;
    if (s < 1) s = 1;
    return (Rectangle){ (float)((sw - GAME_W*s)/2), (float)((sh - GAME_H*s)/2),
                        (float)(GAME_W*s), (float)(GAME_H*s) };
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(GAME_W*3, GAME_H*3, "IRON & INVESTMENT - smithy demo");
    SetTargetFPS(60);

    enum { ATLAS_ROWS = (ATLAS_TILES + ATLAS_COLS - 1) / ATLAS_COLS };
    Texture2D atlas = tex_from_4bpp(atlas_4bpp,
                        ATLAS_COLS*TILE, ATLAS_ROWS*TILE,
                        TILE, TILE, ATLAS_TILES, ATLAS_COLS);
    Texture2D chars = tex_from_4bpp(char_4bpp,
                        CHAR_W*CHAR_FRAMES, CHAR_H,
                        CHAR_W, CHAR_H, CHAR_FRAMES, CHAR_FRAMES);
    RenderTexture2D rt = LoadRenderTexture(GAME_W, GAME_H);
    SetTextureFilter(rt.texture, TEXTURE_FILTER_POINT);

    Player p = { SMITHY_SPAWN_X, SMITHY_SPAWN_Y, DIR_DOWN, false, 0.0f };
    bool debug = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        if (IsKeyPressed(KEY_C))   debug = !debug;
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        /* strict 4-direction input, one axis at a time */
        int dx = 0, dy = 0;
        if      (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) dx = -1;
        else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dx =  1;
        else if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) dy = -1;
        else if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) dy =  1;

        p.moving = (dx | dy) != 0;
        if (p.moving) {
            p.dir = dx < 0 ? DIR_LEFT : dx > 0 ? DIR_RIGHT
                  : dy < 0 ? DIR_UP   : DIR_DOWN;
            p.anim_t += dt;
            float nx = p.x + dx * PLR_SPEED * dt;
            float ny = p.y + dy * PLR_SPEED * dt;
            if (foot_box_free(nx, p.y)) p.x = nx;   /* axis-separated slide */
            if (foot_box_free(p.x, ny)) p.y = ny;
        } else p.anim_t = 0.0f;

        /* camera: horizontal clamp-follow (map wider than view),
         * vertical center (map shorter than view) */
        int cam_x = (int)(p.x - GAME_W/2);
        if (cam_x < 0) cam_x = 0;
        if (cam_x > MAP_PX_W - GAME_W) cam_x = MAP_PX_W - GAME_W;
        int oy = (GAME_H - MAP_PX_H) / 2;

        /* animation: alt toggles every 140 ms while moving */
        int alt = p.moving ? ((int)(p.anim_t / 0.14f) & 1) : 0;
        int frame; bool flip;
        switch (p.dir) {
            case DIR_LEFT:  frame = alt ? 2 : 1; flip = false;      break;
            case DIR_RIGHT: frame = alt ? 2 : 1; flip = true;       break;
            case DIR_UP:    frame = 3;           flip = (bool)alt;  break;
            default:        frame = 0;           flip = (bool)alt;  break;
        }

        BeginTextureMode(rt);
            ClearBackground((Color){ 23, 21, 31, 255 });  /* void */
            map_draw(atlas, -cam_x, oy);

            Rectangle src = { (float)(frame*CHAR_W), 0.0f,
                              flip ? -(float)CHAR_W : (float)CHAR_W,
                              (float)CHAR_H };
            int sx = (int)p.x - CHAR_W/2 - cam_x;
            int sy = (int)p.y - CHAR_H + oy - alt;    /* 1px bob */
            DrawTextureRec(chars, src, (Vector2){ (float)sx, (float)sy }, WHITE);

            if (debug) {
                for (int ty = 0; ty < SMITHY_H; ty++)
                    for (int tx = 0; tx < SMITHY_W; tx++)
                        if (SMITHY_SOLID(tx, ty))
                            DrawRectangle(tx*TILE - cam_x, ty*TILE + oy,
                                          TILE, TILE, (Color){255,60,60,90});
                DrawRectangle((int)p.x - FOOT_W/2 - cam_x,
                              (int)p.y - FOOT_H + oy,
                              FOOT_W, FOOT_H, (Color){60,255,120,200});
            }
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(rt.texture,
                (Rectangle){ 0, 0, (float)GAME_W, -(float)GAME_H },
                pillarbox_dst(), (Vector2){0,0}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(rt);
    UnloadTexture(chars);
    UnloadTexture(atlas);
    CloseWindow();
    return 0;
}
