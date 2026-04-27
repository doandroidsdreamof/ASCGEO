#include "terminal.h"
#include <signal.h>
#include "map.h"
#include "connection.h"

//* feature ideas:
/*
 - bandwidth per connection
 - export/snapshot as JSON
 - connection timeline
 - passive OS fingerprinting of remote hosts
 - DNS leak detection
 - dependency connection audit
 - docker/container network inspector
 - database connection pool monitor
*/

WINDOW *win_map = NULL;
WINDOW *win_conn = NULL;
WINDOW *win_stats = NULL;

static volatile bool needs_resize = false;
FocusPanel focused_panel = FOCUS_NONE;
int active_panel = 0;

void terminal_set_focus(FocusPanel panel)
{
    focused_panel = panel;
    needs_resize = true;
}

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
    init_pair(4, COLOR_BLUE, -1);   // active panel border

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

    int map_rows, conn_rows, stats_rows;

    switch (focused_panel)
    {
    case FOCUS_MAP:
        map_rows = rows;
        conn_rows = 0;
        stats_rows = 0;
        break;
    case FOCUS_CONN:
        map_rows = 0;
        conn_rows = rows;
        stats_rows = 0;
        break;
    case FOCUS_STATS:
        map_rows = 0;
        conn_rows = 0;
        stats_rows = rows;

        break;
    default:
        // split: 60% map, 25% connections, 15% stats
        map_rows = rows * 60 / 100;
        conn_rows = rows * 25 / 100;
        stats_rows = rows - map_rows - conn_rows;
        if (stats_rows < 3)
            stats_rows = 3;
        break;
    }

    if (win_map)
        delwin(win_map);
    if (win_conn)
        delwin(win_conn);
    if (win_stats)
        delwin(win_stats);

    win_map = NULL;
    win_conn = NULL;
    win_stats = NULL;

    int y = 0;
    if (map_rows > 0)
    {
        win_map = newwin(map_rows, cols, y, 0);
        box(win_map, 0, 0);
        wrefresh(win_map);
        y += map_rows;
    }
    if (conn_rows > 0)
    {
        win_conn = newwin(conn_rows, cols, y, 0);
        box(win_conn, 0, 0);
        wrefresh(win_conn);
        y += conn_rows;
    }
    if (stats_rows > 0)
    {
        win_stats = newwin(stats_rows, cols, y, 0);
        box(win_stats, 0, 0);
        // mvwprintw(win_stats, 1, 2, "Connections: 12  | TCP: 10  | UDP: 2");
        // mvwprintw(win_stats, 1, 2, "Connections: %d", connection_count());
        wrefresh(win_stats);
        y += stats_rows;
    }
    map_mark_dirty();
    connection_mark_dirty();
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