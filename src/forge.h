/* forge.h - BEST's own counter: FORGE makes things, BLUEPRINTS reads about
   them.

   Deliberately not a UiMenu screen, for the same reason the shop is not: the
   menu's frames are a navigation stack over static tables, and this screen
   mutates the pack, keeps a cursor across a tab switch and carries a result
   line. It is also deliberately not two screens. FORGE and BLUEPRINTS are the
   same list of the same recipes with the same detail pane; the only
   differences are whether the category tabs are live and whether the accept
   key does anything. Two files would have been two copies of the row builder,
   the material readout and the scroll clamp so that one of them could refuse
   a key press.

   Unavailable recipes are shown, not hidden. A menu that changes length
   depending on what is in the pack cannot be learned - the player has no way
   to tell "I cannot afford this" from "this does not exist", and the row that
   vanished is exactly the one they were about to plan around. Disabled rows
   keep their place and the detail pane says which ingredient is short and by
   how much. This is the same call the title screen makes when it greys LOAD
   rather than removing it.

   1.6 filled in the seam 1.5 left. The accept key no longer resolves a craft;
   it spends nothing and returns a command naming the recipe, and the scene
   takes the ore and starts the minigame. That mirrors UiMenu handing TALK and
   travel back rather than acting on them: this screen owns a cursor and a
   list, and the world state belongs to whoever owns the room. */
#ifndef FORGE_H
#define FORGE_H

#include "ui.h"
#include "inventory.h"
#include "sort.h"

/* Commands handed back to the scene. A craft is encoded as a base plus the
   recipe index rather than an out parameter, for the same reason MenuCommand
   encodes travel that way: every other command is a bare enum, and adding a
   payload pointer would complicate all three call sites for the benefit of
   one. */
#define FORGE_CMD_BEGIN_BASE 16

typedef enum ForgeCommand {
    FORGE_CMD_NONE = 0
} ForgeCommand;

#define ForgeCmdIsBegin(c) ((c) >= FORGE_CMD_BEGIN_BASE)
#define ForgeCmdRecipe(c)  ((int)((c) - FORGE_CMD_BEGIN_BASE))

typedef enum ForgeMode {
    FORGE_MODE_CRAFT = 0,   /* FORGE: tabs live, accept forges     */
    FORGE_MODE_BOOK         /* BLUEPRINTS: every known recipe, read-only */
} ForgeMode;

typedef struct UiForge {
    signed char mode;       /* ForgeMode */
    signed char tab;        /* ForgeCat, craft mode only */
    signed char cursor;
    signed char scroll;
    SortState sort;
    bool open;
    const char *result;
    float result_time;
} UiForge;

void ForgeInit(UiForge *f);

bool ForgeIsOpen(const UiForge *f);

/* A fresh visit: tab, cursor and sort go back to the top. Blueprints opens
   the same widget with the tabs inert. */
void ForgeOpen(UiForge *f, ForgeMode mode);
void ForgeClose(UiForge *f);

void ForgeUpdate(UiForge *f, float dt);

/* One input step. dx switches category, dy moves the cursor, accept asks the
   scene to start a heat. */
ForgeCommand ForgeInput(UiForge *f, int dx, int dy, bool accept);

/* Reopened after a heat, with the verdict as its result line. Cursor, shelf
   and sort are left alone: from the player's side they never left the list,
   and the commonest thing to do after forging one blade is to forge another. */
void ForgeResume(UiForge *f, const char *result);

void ForgeSort(UiForge *f, SortMode mode);

void ForgeDraw(const UiForge *f);

#endif /* FORGE_H */
