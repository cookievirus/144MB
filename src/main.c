/* IRON & INVESTMENT - Demo 1.4
   A title screen, two rooms driven from one scene table, travel between them,
   and a trader who buys and sells.

   Unity build: this is the only translation unit. */

#include "raylib.h"

#include <stdio.h>

#include "gfx.c"
#include "rarity.c"
#include "ui_font.c"
#include "ui.c"
#include "sort.c"
#include "inventory.c"
#include "shop.c"
#include "ui_menu.c"
#include "ui_prompt.c"
#include "ui_dialog.c"
#include "scene.c"
#include "title.c"

#define WINDOW_SCALE 3
#define DT_CLAMP 0.10f   /* stops a dragged/paused window from teleporting */

/* Two modes rather than another layer on the scene's stack. The title is not
   drawn over the game - while it is up the game does not exist yet, and its
   room is only loaded once START or LOAD is chosen. That is what keeps a save
   from being resumed on top of a world that has already started. */
typedef enum AppMode {
    APP_TITLE = 0,
    APP_GAME
} AppMode;

/* Accept is one action with three keys, defined in one place so the dialogue,
   the menu and the title can never drift apart on what "confirm" means. */
static bool AcceptPressed(void)
{
    return IsKeyPressed(KEY_SPACE) ||
           IsKeyPressed(KEY_ENTER) ||
           IsKeyPressed(KEY_KP_ENTER);
}

int main(void)
{
    printf("IRON & INVESTMENT - Demo 1.4\n");
    printf("Backbuffer %dx%d | Window %dx%d | Target %d FPS\n",
           VSCREEN_W, VSCREEN_H, VSCREEN_W * WINDOW_SCALE, VSCREEN_H * WINDOW_SCALE, 60);
    printf("Keys: M=menu Arrows=move Space/Enter=accept ESC=back F5=replay F11=fullscreen\n");
    printf("Sort (inventory / shop): R=rarity T=quantity A=name, same key flips\n");
    printf("Travel: M -> MAP -> Market Row\n");
    fflush(stdout);
    SetTraceLogLevel(LOG_INFO);
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(VSCREEN_W * WINDOW_SCALE, VSCREEN_H * WINDOW_SCALE,
               "IRON & INVESTMENT - Demo 1.4");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);   /* ESC is handled explicitly below */

    RenderTexture2D backbuffer = LoadRenderTexture(VSCREEN_W, VSCREEN_H);
    SetTextureFilter(backbuffer.texture, TEXTURE_FILTER_POINT);

    UiFontLoad();
    RarityLoad();

    AppMode mode = APP_TITLE;
    bool running = true;

    UiTitle title;
    TitleLoad(&title);

    Scene scene;
    bool scene_live = false;

    while (running && !WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > DT_CLAMP) dt = DT_CLAMP;

        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        if (mode == APP_TITLE) {
            if (IsKeyPressed(KEY_UP))   TitleMove(&title, -1);
            if (IsKeyPressed(KEY_DOWN)) TitleMove(&title,  1);

            if (AcceptPressed()) {
                switch (TitleAccept(&title)) {
                case TITLE_START:
                    SceneInit(&scene, SCENE_SMITHY);
                    scene_live = true;
                    mode = APP_GAME;
                    break;
                case TITLE_LOAD:
                    SceneInit(&scene, SCENE_SMITHY);
                    scene_live = true;
                    /* SceneInit has already seeded a fresh game, so a load
                       that fails here leaves a playable world rather than
                       half of one. */
                    if (!SceneLoadSave(&scene)) title.note = "Save is unreadable.";
                    mode = APP_GAME;
                    break;
                case TITLE_EXIT:
                    running = false;
                    break;
                default:
                    break;   /* OPTION and a refused LOAD set their own note */
                }
            }

            /* Released as the game starts: the smithy loads its own copy of
               the same image, and holding both would keep a texture alive
               that nothing draws. */
            if (mode == APP_GAME) TitleUnload(&title);

            TitleUpdate(&title, dt);
        } else {
            /* ESC is purely a "back" key. It closes one layer, and once
               nothing is open it raises the system prompt; only a confirmed
               QUIT there ends the process. */
            if (IsKeyPressed(KEY_ESCAPE)) SceneBack(&scene);
            if (IsKeyPressed(KEY_M)) SceneToggleMenu(&scene);
            /* Replay moved off R in 1.3. R now sorts, and a key that means
               "reorder this list" on one screen and "throw the session away"
               on another is the kind of ambiguity this codebase keeps out of
               the input layer. */
            if (IsKeyPressed(KEY_F5)) SceneReset(&scene);
            if (IsKeyPressed(KEY_R)) SceneSort(&scene, SORT_RARITY);
            if (IsKeyPressed(KEY_T)) SceneSort(&scene, SORT_QTY);
            if (IsKeyPressed(KEY_A)) SceneSort(&scene, SORT_ALPHA);
            if (AcceptPressed()) SceneAdvance(&scene);

            if (IsKeyPressed(KEY_LEFT))  SceneMove(&scene, -1, 0);
            if (IsKeyPressed(KEY_RIGHT)) SceneMove(&scene,  1, 0);
            if (IsKeyPressed(KEY_UP))    SceneMove(&scene,  0, -1);
            if (IsKeyPressed(KEY_DOWN))  SceneMove(&scene,  0,  1);

            SceneUpdate(&scene, dt);
            if (SceneWantsQuit(&scene)) running = false;
        }

        BeginTextureMode(backbuffer);
            ClearBackground(BLACK);
            if (mode == APP_TITLE) TitleDraw(&title);
            else                   SceneDraw(&scene);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(backbuffer.texture, GfxRenderTextureSrc(),
                           GfxPillarboxDst(), (Vector2){ 0.0f, 0.0f },
                           0.0f, WHITE);
        EndDrawing();
    }

    if (scene_live) SceneUnload(&scene);
    else            TitleUnload(&title);
    RarityUnload();
    UiFontUnload();
    UnloadRenderTexture(backbuffer);
    CloseWindow();
    return 0;
}
