#ifndef TERMINAL_H
#define TERMINAL_H

#include <ncurses.h>

typedef enum {
    FOCUS_NONE,
    FOCUS_MAP,
    FOCUS_CONN,
    FOCUS_STATS
} FocusPanel;

extern FocusPanel focused_panel;
extern int active_panel;

extern WINDOW *win_map;
extern WINDOW *win_conn;
extern WINDOW *win_stats;

void terminal_init(void);
void terminal_cleanup(void);
void terminal_layout(void);
int terminal_rows(void);
int terminal_cols(void);
void terminal_set_focus(FocusPanel panel);

#endif