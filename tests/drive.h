/* drive.h - reach a feature the way a player now has to: through the menu.

   Included *after* scene.c, so it can see RootBuild and the ROOT_* row ids
   that ui_menu.c keeps static. That is the same bargain the suites already
   make by linking src/*.c directly rather than through main.c.

   1.7 is why this file exists. Four suites opened the counter by pressing the
   accept key at an idle screen, because until 1.7 that worked; when the only
   way in became the main menu, four suites broke in four different places for
   one reason. Naming the row and letting the helper find it means the next
   time the grid gains an entry, nothing here moves. */
#ifndef TEST_DRIVE_H
#define TEST_DRIVE_H

/* Put the cursor on a named root row and accept it. Returns false if this
   room does not offer that row at all - which is itself worth asserting.

   The cursor is placed rather than walked to. Walking it would retest the
   grid navigation at every call site, and that has one suite of its own. */
static bool DriveRoot(Scene *s, int root_row)
{
    if (!UiMenuIsOpen(&s->menu)) SceneToggleMenu(s);
    if (!UiMenuIsOpen(&s->menu)) return false;

    unsigned char rows[ROOT_MAX_ROWS];
    const int n = RootBuild(&s->menu, rows);

    for (int i = 0; i < n; i++) {
        if ((int)rows[i] != root_row) continue;
        s->menu.stack[s->menu.depth - 1].cursor = (signed char)i;
        SceneAdvance(s);
        return true;
    }
    return false;
}

/* The room's own feature: the counter, the anvil, the roster. */
static bool DriveFeature(Scene *s) { return DriveRoot(s, ROOT_FEATURE); }

/* Let a script run to its end so the room goes idle. */
static void DriveSkipDialog(Scene *s)
{
    for (int i = 0; i < 64 && s->dialog.phase != DIALOG_HIDDEN; i++) {
        s->dialog.shown = s->dialog.total;
        SceneAdvance(s);
    }
    UiDialogHide(&s->dialog);
}

#endif /* TEST_DRIVE_H */
