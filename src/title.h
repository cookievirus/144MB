/* title.h - the screen before the game.

   Deliberately not a Scene. A room is a backdrop plus actors plus a script
   with an entrance state machine behind it, and the title has none of those:
   it is one still image and four words. Making it a SceneDef row would mean
   teaching SceneDef about menus that are not the command menu, and teaching
   the scene's ESC ladder about a screen that has nothing to go back to.

   The backdrop is a placeholder - the forge, reused. It costs nothing: the
   asset is already embedded for the smithy, so the title screen adds a
   texture handle and no payload at all. When real title art arrives it is one
   #include and one field. */
#ifndef TITLE_H
#define TITLE_H

#include "gfx.h"

typedef enum TitleCmd {
    TITLE_NONE = 0,
    TITLE_START,
    TITLE_LOAD,
    TITLE_OPTION,
    TITLE_EXIT
} TitleCmd;

#define TITLE_ROWS 4

typedef struct UiTitle {
    Texture2D bg;
    signed char cursor;
    bool has_save;        /* LOAD is greyed out and refuses without one */
    const char *note;     /* one line under the menu; NULL for none */
    float clock;          /* drives the caret blink only */
} UiTitle;

/* Loads the placeholder backdrop; call after InitWindow(). */
void TitleLoad(UiTitle *t);
void TitleUnload(UiTitle *t);

/* Re-checks whether a save exists. Called on load and whenever the player
   comes back to the title, so LOAD is never offered for a file that is not
   there and never withheld for one that is. */
void TitleRefresh(UiTitle *t);

void TitleMove(UiTitle *t, int dy);
TitleCmd TitleAccept(UiTitle *t);

void TitleUpdate(UiTitle *t, float dt);
void TitleDraw(const UiTitle *t);

#endif /* TITLE_H */
