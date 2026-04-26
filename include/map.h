#ifndef MAP_H
#define MAP_H

#define MAP_MAX_ROWS 60
#define MAP_MAX_COLS 200

int map_load(const char *filepath, int cols, int rows);
void map_render(void);
void map_update(void); 
void map_mark_dirty(void);
int map_current_rows(void);
int map_current_cols(void);

#endif