/* scene_smithy.h - Demo 1.0 scene: static forge backdrop, hero slides in. */
#ifndef SCENE_SMITHY_H
#define SCENE_SMITHY_H

#include "gfx.h"
#include "ui_dialog.h"
#include "ui_menu.h"
#include "ui_prompt.h"

typedef enum SmithyPhase {
    SMITHY_HOLD,     /* empty room, lets the backdrop read before entry */
    SMITHY_ENTER,    /* hero eases in from the right                    */
    SMITHY_SETTLED,  /* hero at rest, dialogue running                  */
    SMITHY_DONE      /* script finished                                 */
} SmithyPhase;

typedef struct SceneSmithy {
    Texture2D bg;
    Texture2D hero;
    SmithyPhase phase;
    float clock;     /* seconds elapsed inside the current phase */
    float hero_x;    /* virtual-screen pixels, left edge of the sprite */
    UiDialog dialog;
    UiMenu menu;
    UiPrompt prompt;
    const DialogLine *script;  /* line array currently playing */
    int script_len;
    int script_at;
    bool quit_requested;
} SceneSmithy;

void SceneSmithyInit(SceneSmithy *s);
void SceneSmithyReset(SceneSmithy *s);
void SceneSmithyUpdate(SceneSmithy *s, float dt);
void SceneSmithyAdvance(SceneSmithy *s);   /* advance key: dialogue or menu */
void SceneSmithyToggleMenu(SceneSmithy *s);
void SceneSmithyBack(SceneSmithy *s);              /* ESC: one level, always */
void SceneSmithyMove(SceneSmithy *s, int dx, int dy);

/* True once the player has confirmed QUIT in the system prompt. Nothing else
   in the game can end the process. */
bool SceneSmithyWantsQuit(const SceneSmithy *s);
void SceneSmithyDraw(const SceneSmithy *s);
void SceneSmithyUnload(SceneSmithy *s);

#endif /* SCENE_SMITHY_H */
