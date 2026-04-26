#include "terminal.h"
#include <signal.h>
#include "map.h"

WINDOW *win_map = NULL;
WINDOW *win_conn = NULL;
WINDOW *win_stats = NULL;

static volatile bool needs_resize = false;

static void handle_resize(int sig)
{
    (void)sig;
    needs_resize = true;
}

void terminal_init(void)
{
    initscr();             // start ncurses
    cbreak();              // disable line buffering (keys sent immediately)
    noecho();              // don't print typed keys
    curs_set(0);           // hide cursor
    nodelay(stdscr, TRUE); // non-blocking getch (so game loop doesn't stall)
    start_color();
    use_default_colors();
    init_pair(1, COLOR_CYAN, -1);   // ASCGEO title
    init_pair(2, COLOR_CYAN, -1);   // subtitle
    init_pair(3, COLOR_YELLOW, -1); // menu items

    signal(SIGWINCH, handle_resize);
    needs_resize = true; // force initial layout
    terminal_layout();
}

void terminal_cleanup(void)
{
    if (win_map)
        delwin(win_map);
    if (win_conn)
        delwin(win_conn);
    if (win_stats)
        delwin(win_stats);
    endwin(); // restore terminal to normal
}

void terminal_layout(void)
{
    if (!needs_resize)
        return;

    // endwin(); => it causes duplicated layout screens
    refresh();

    int rows = LINES; // ncurses global: terminal height
    int cols = COLS;  // ncurses global: terminal width

    // split: 70% map, 25% connections, 5% 
    int map_rows = rows * 70 / 100;
    int conn_rows = rows * 25 / 100;
    int stats_rows = rows - map_rows - conn_rows;
    if (stats_rows < 2)
        stats_rows = 2;

    // destroy old windows
    if (win_map)
        delwin(win_map);
    if (win_conn)
        delwin(win_conn);
    if (win_stats)
        delwin(win_stats);

    // newwin(height, width, start_row, start_col)
    win_map = newwin(map_rows, cols, 0, 0);
    win_conn = newwin(conn_rows, cols, map_rows, 0);
    win_stats = newwin(stats_rows, cols, map_rows + conn_rows, 0);

    box(win_map, 0, 0);
    box(win_conn, 0, 0);
    box(win_stats, 0, 0);

    mvwprintw(win_stats, 0, 2, " STATISTICS ");
    mvwprintw(win_stats, 1, 2, "Connections: 12  | TCP: 10  | UDP: 2");

    wrefresh(win_map);
    wrefresh(win_conn);
    wrefresh(win_stats);

    map_mark_dirty();
    needs_resize = false;
}

int terminal_rows(void)
{
    return LINES;
}

int terminal_cols(void)
{
    return COLS;
}