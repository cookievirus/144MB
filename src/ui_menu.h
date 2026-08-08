/* ui_menu.h - the command window and its sub-screens.

   Navigation is a stack, not a state machine with hand-written transitions:
   ESC pops exactly one frame, always, and popping the last frame closes the
   menu. Adding a screen therefore cannot break the back behaviour. */
#ifndef UI_MENU_H
#define UI_MENU_H

#include "ui.h"
#include "game_data.h"
#include "inventory.h"
#include "sort.h"

#define MENU_MAX_DEPTH 4

typedef enum MenuScreen {
    SCREEN_ROOT = 0,
    SCREEN_INVENTORY,
    SCREEN_EQUIPMENT,
    SCREEN_MAP
} MenuScreen;

/* Commands the menu hands back to the scene, which owns the world state.

   Travel is encoded as a base plus a SceneId rather than a separate return
   parameter, because every other command is a bare enum and adding an out
   pointer to UiMenuInput would complicate all four call sites to carry a
   payload one of them uses. */
#define MENU_CMD_TRAVEL_BASE 16

typedef enum MenuCommand {
    MENU_CMD_NONE = 0,
    MENU_CMD_TALK,
    /* 1.7. The room's own feature - the counter, the anvil, the roster. The
       menu does not know which of those it is; it knows the room declared
       one, draws its label and hands the fact back. The scene owns the
       switch, exactly as it owns TALK and travel. */
    MENU_CMD_FEATURE,
    MENU_CMD_END_DAY
} MenuCommand;

#define MenuCmdIsTravel(c) ((c) >= MENU_CMD_TRAVEL_BASE)
#define MenuCmdScene(c)    ((int)((c) - MENU_CMD_TRAVEL_BASE))

typedef struct MenuFrame {
    unsigned char screen;
    signed char cursor;   /* row, or cell index on a grid screen */
    signed char tab;      /* inventory category */
    signed char scroll;   /* first visible row */
} MenuFrame;

typedef struct UiMenu {
    MenuFrame stack[MENU_MAX_DEPTH];
    int depth;            /* 0 = closed */
    SortState sort;       /* inventory ordering, kept across opens */
    /* RoomFeature of the room the menu was opened in. Copied in at open
       rather than read from SCENES, so ui_menu.c never includes scene.h and
       the tests can drive a root menu without standing up a room. */
    unsigned char feature;
} UiMenu;

/* True when a sort key should reach the menu, i.e. a list screen is on top. */
bool UiMenuTakesSort(const UiMenu *m);

void UiMenuInit(UiMenu *m);
bool UiMenuIsOpen(const UiMenu *m);
void UiMenuOpen(UiMenu *m, int feature);
void UiMenuClose(UiMenu *m);

/* One input step. dx/dy are -1, 0 or 1. Returns a command for the scene. */
MenuCommand UiMenuInput(UiMenu *m, int dx, int dy, bool accept, bool back);

void UiMenuDraw(const UiMenu *m);

#endif /* UI_MENU_H */
