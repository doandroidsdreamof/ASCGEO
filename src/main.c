#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <ncurses.h>
#include "terminal.h"
#include "map.h"
#include "connection.h"
#include <string.h>

typedef enum
{
    MODE_MENU,
    MODE_GEOMAP,
    MODE_TRACEROUTE,
    MODE_NETMAP
} AppMode;

static int menu_selection = 0;
static const int MENU_ITEMS = 3;

static void render_menu(void)
{
    clear();

    int rows = LINES;
    int cols = COLS;

    const char *art[] = {
        "  ___   _____ _____ _____  _____ _____ ",
        " / _ \\ /  ___/  __ \\  __ \\|  ___|  _  |",
        "/ /_\\ \\\\ `--.| /  \\/ |  \\/| |__ | | | |",
        "|  _  | `--. \\ |   | | __ |  __|| | | |",
        "| | | |/\\__/ / \\__/\\ |_\\ \\| |___\\ \\_/ /",
        "\\_| |_/\\____/ \\____/\\____/\\____/ \\___/ ",
    };
    int art_lines = 6;
    int art_width = 42;
    int start_y = (rows / 2) - 6;
    int start_x = (cols - art_width) / 2;
    const char *subtitle = "Network Visualization Tool";

    attron(COLOR_PAIR(1) | A_BOLD);
    for (int i = 0; i < art_lines; i++)
    {
        mvprintw(start_y + i, start_x, "%s", art[i]);
    }
    attroff(COLOR_PAIR(1) | A_BOLD);

    attron(COLOR_PAIR(2));
    mvprintw(start_y + art_lines + 1, (cols - (int)strlen(subtitle)) / 2, "%s", subtitle);
    attroff(COLOR_PAIR(2));
    mvprintw(start_y + art_lines + 1, (cols - (int)strlen(subtitle)) / 2, "%s", subtitle);

    const char *options[] = {"GEOMAP", "TRACEROUTE", "NETMAP"};
    int menu_y = start_y + art_lines + 4;

    for (int i = 0; i < MENU_ITEMS; i++)
    {
        char label[64];
        if (i == menu_selection)
        {
            snprintf(label, sizeof(label), "  > %s <  ", options[i]);
            attron(A_REVERSE);
            mvprintw(menu_y + i, (cols - (int)strlen(label)) / 2, "%s", label);
            attroff(A_REVERSE);
        }
        else
        {
            snprintf(label, sizeof(label), "    %s    ", options[i]);
            mvprintw(menu_y + i, (cols - (int)strlen(label)) / 2, "%s", label);
        }
    }

    const char *footer = "UP/DOWN to navigate  |  ENTER to select  |  q to quit";
    mvprintw(menu_y + 5, (cols - (int)strlen(footer)) / 2, "%s", footer);

    refresh();
}

static void render_placeholder(const char *title)
{
    clear();
    int rows = LINES;
    int cols = COLS;

    mvprintw(rows / 2 - 1, (cols - (int)strlen(title)) / 2, "%s", title);

    const char *msg = "Coming soon...";
    mvprintw(rows / 2 + 1, (cols - (int)strlen(msg)) / 2, "%s", msg);

    const char *back = "Press ESC to go back";
    mvprintw(rows / 2 + 3, (cols - (int)strlen(back)) / 2, "%s", back);

    refresh();
}

int main(void)
{
    terminal_init();
    keypad(stdscr, TRUE); // enable arrow keys

    AppMode mode = MODE_MENU;
    bool running = true;

    while (running)
    {
        int ch = getch();
        if (ch == 'q')
        {
            running = false;
            break;
        }

        switch (mode)
        {
        case MODE_MENU:
            if (ch == KEY_UP)
            {
                menu_selection = (menu_selection - 1 + MENU_ITEMS) % MENU_ITEMS;
            }
            if (ch == KEY_DOWN)
            {
                menu_selection = (menu_selection + 1) % MENU_ITEMS;
            }
            if (ch == '\n' || ch == KEY_ENTER)
            {
                clear();
                refresh();
                if (menu_selection == 0)
                    mode = MODE_GEOMAP;
                if (menu_selection == 1)
                    mode = MODE_TRACEROUTE;
                if (menu_selection == 2)
                    mode = MODE_NETMAP;
                break;
            }
            render_menu();
            break;

        case MODE_GEOMAP:
            if (ch == 27)
            {
                mode = MODE_MENU;
                clear();
                refresh();
                break;
            }
            terminal_layout();
            map_update();
            connection_update();
            map_render();
            connection_render();
            break;

        case MODE_TRACEROUTE:
            render_placeholder("TRACEROUTE");
            if (ch == 27)
            {
                mode = MODE_MENU;
                clear();
                refresh();
            }
            break;

        case MODE_NETMAP:
            render_placeholder("NETMAP");
            if (ch == 27)
            {
                mode = MODE_MENU;
                clear();
                refresh();
            }
            break;
        }

        usleep(100000);
    }

    terminal_cleanup();
    return 0;
}