#ifndef EXPLORER_H
#define EXPLORER_H

#include "vi.h"

bool is_directory(const char *path);
bool explorer_open(const char *path);
bool explorer_handle_enter(void);
bool explorer_find_file(const char *query, char *out_path, int out_size);

#endif /* EXPLORER_H */
