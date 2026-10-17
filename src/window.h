#ifndef WINDOW_H
#define WINDOW_H

#include "vi.h"

void window_init(void);
bool window_split(bool vertical);
bool window_close_current(void);
void window_next(void);
void window_prev(void);
void window_move_dir(char dir);
void window_balance(void);
void window_save_active(void);
void window_load_active(int idx);
int window_find_at(int screen_x, int screen_y);

#endif /* WINDOW_H */
