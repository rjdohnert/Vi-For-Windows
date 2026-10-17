#ifndef TAB_H
#define TAB_H

#include "vi.h"

void tab_init(void);
bool tab_new(const char *filename);
bool tab_close(void);
void tab_next(void);
void tab_prev(void);
void tab_switch(int idx);
void tab_only(void);
void tab_save_current(void);
void tab_load(int idx);
bool tab_handle_click(int mouse_x);

#endif /* TAB_H */
