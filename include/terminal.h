#ifndef TERMINAL_H
#define TERMINAL_H

#include <ncurses.h>

// windows
extern WINDOW *win_map;
extern WINDOW *win_conn;
extern WINDOW *win_stats;

void terminal_init(void);
void terminal_cleanup(void);
void terminal_layout(void);
int terminal_rows(void);
int terminal_cols(void);

#endif