#ifndef FOLD_H
#define FOLD_H

#include "vi.h"

void fold_init(void);
void fold_update(void);
bool fold_close_at(int cy);
bool fold_open_at(int cy);
bool fold_toggle_at(int cy);
void fold_open_all(void);
void fold_close_all(void);
Fold *fold_find_at(int cy);
Fold *fold_find_start(int cy);
bool fold_is_line_hidden(int cy);

#endif /* FOLD_H */
