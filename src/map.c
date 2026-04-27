#include "map.h"
#include "terminal.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

static char map_buffer[MAP_MAX_ROWS][MAP_MAX_COLS + 1];
static bool dirty = true;
static int current_rows = 0;
static int current_cols = 0;
static const char *current_map_path = NULL;

int map_load(const char *filepath, int cols, int rows)
{
    if (cols > MAP_MAX_COLS)
        cols = MAP_MAX_COLS;
    if (rows > MAP_MAX_ROWS)
        rows = MAP_MAX_ROWS;

    memset(map_buffer, ' ', sizeof(map_buffer));

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL)
        return -1;

    int row = 0;
    char line[MAP_MAX_COLS + 2];
    while (row < rows && fgets(line, sizeof(line), fp) != NULL)
    {
        line[strcspn(line, "\n")] = '\0';
        int len = strlen(line);
        if (len > cols)
            len = cols;
        memcpy(map_buffer[row], line, len);
        map_buffer[row][cols] = '\0';
        row++;
    }
    fclose(fp);

    current_rows = row;
    current_cols = cols;
    dirty = true;
    return 0;
}

void map_update(void)
{
    // TODO implement dynamic
    int cols = terminal_cols();
    const char *new_path;
    int map_cols, map_rows;

    if (cols >= 200)
    {
        new_path = "assets/worldmap_200.txt";
        map_cols = 200;
        map_rows = 60;
    }
    else if (cols >= 160)
    {
        new_path = "assets/worldmap_160.txt";
        map_cols = 160;
        map_rows = 48;
    }
    else if (cols >= 120)
    {
        new_path = "assets/worldmap_120.txt";
        map_cols = 120;
        map_rows = 36;
    }
    else
    {
        new_path = "assets/worldmap_80.txt";
        map_cols = 80;
        map_rows = 24;
    }

    if (current_map_path != new_path || current_cols != map_cols)
    {
        map_load(new_path, map_cols, map_rows);
        current_map_path = new_path;
    }
}

void map_render(void)
{
    if (!dirty || !win_map)
        return;

    werase(win_map);
    if (active_panel == 0 && focused_panel == FOCUS_NONE)
        wattron(win_map, COLOR_PAIR(4));
    box(win_map, 0, 0);
    if (active_panel == 0 && focused_panel == FOCUS_NONE)
        wattroff(win_map, COLOR_PAIR(4));
    mvwprintw(win_map, 0, 2, " WORLD MAP ");

    int win_rows, win_cols;
    getmaxyx(win_map, win_rows, win_cols);

    int inner_rows = win_rows - 2;
    int inner_cols = win_cols - 2;

    for (int y = 1; y <= inner_rows; y++)
    {
        for (int x = 1; x <= inner_cols; x++)
        {
            mvwaddch(win_map, y, x, '.');
        }
    }

    int rows_to_draw = (current_rows < inner_rows) ? current_rows : inner_rows;

    // Offset by +1 to stay inside the box
    int pad_left = (inner_cols > current_cols) ? (inner_cols - current_cols) / 2 : 0;
    int pad_top = (inner_rows > current_rows) ? (inner_rows - current_rows) / 2 : 0;

    for (int i = 0; i < rows_to_draw; i++)
    {
        // Print starting at pad_top + 1, pad_left + 1
        mvwprintw(win_map, pad_top + 1 + i, pad_left + 1, "%.*s",
                  inner_cols - pad_left, map_buffer[i]);
    }

    wrefresh(win_map);
    dirty = false;
}

void map_mark_dirty(void)
{
    dirty = true;
}

int map_current_rows(void) { return current_rows; }
int map_current_cols(void) { return current_cols; }