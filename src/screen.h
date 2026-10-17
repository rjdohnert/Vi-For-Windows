#ifndef SCREEN_H
#define SCREEN_H

#include "vi.h"

int row_cx_to_rx(Line *row, int cx);
void editor_scroll(void);
void render_help(void);
void editor_refresh_screen(void);
void set_status(const char *fmt, ...);

#endif /* SCREEN_H */
