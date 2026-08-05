/* IRON & INVESTMENT - Demo 1.0
   Static forge backdrop + hero entering from the right.

   Unity build: this is the only translation unit. */

#include "raylib.h"

#include "gfx.c"
#include "ui.c"
#include "ui_font.c"
#include "ui_menu.c"
#include "ui_prompt.c"
#include "ui_dialog.c"
#include "scene_smithy.c"

#define WINDOW_SCALE 3
#define DT_CLAMP 0.10f   /* stops a dragged/paused window from teleporting */

/* Accept is one action with three keys, defined in one place so the dialogue
   and the menu can never drift apart on what "confirm" means. */
static bool AcceptPressed(void)
{
    return IsKeyPressed(KEY_SPACE) ||
           IsKeyPressed(KEY_ENTER) ||
           IsKeyPressed(KEY_KP_ENTER);
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(VSCREEN_W * WINDOW_SCALE, VSCREEN_H * WINDOW_SCALE,
               "IRON & INVESTMENT - Demo 1.0");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);   /* ESC is handled explicitly below */

    RenderTexture2D backbuffer = LoadRenderTexture(VSCREEN_W, VSCREEN_H);
    SetTextureFilter(backbuffer.texture, TEXTURE_FILTER_POINT);

    UiFontLoad();

    SceneSmithy scene;
    SceneSmithyInit(&scene);

    while (!WindowShouldClose()) {
        /* ESC is purely a "back" key. It closes one layer, and once nothing
           is open it raises the system prompt; only a confirmed QUIT there
           ends the process. */
        if (IsKeyPressed(KEY_ESCAPE)) SceneSmithyBack(&scene);
        if (IsKeyPressed(KEY_M)) SceneSmithyToggleMenu(&scene);
        if (IsKeyPressed(KEY_R)) SceneSmithyReset(&scene);
        if (AcceptPressed()) SceneSmithyAdvance(&scene);
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        if (IsKeyPressed(KEY_LEFT))  SceneSmithyMove(&scene, -1, 0);
        if (IsKeyPressed(KEY_RIGHT)) SceneSmithyMove(&scene,  1, 0);
        if (IsKeyPressed(KEY_UP))    SceneSmithyMove(&scene,  0, -1);
        if (IsKeyPressed(KEY_DOWN))  SceneSmithyMove(&scene,  0,  1);

        float dt = GetFrameTime();
        if (dt > DT_CLAMP) dt = DT_CLAMP;
        SceneSmithyUpdate(&scene, dt);
        if (SceneSmithyWantsQuit(&scene)) break;

        BeginTextureMode(backbuffer);
            ClearBackground(BLACK);
            SceneSmithyDraw(&scene);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(backbuffer.texture, GfxRenderTextureSrc(),
                           GfxPillarboxDst(), (Vector2){ 0.0f, 0.0f },
                           0.0f, WHITE);
        EndDrawing();
    }

    SceneSmithyUnload(&scene);
    UiFontUnload();
    UnloadRenderTexture(backbuffer);
    CloseWindow();
    return 0;
}
