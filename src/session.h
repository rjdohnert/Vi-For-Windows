#ifndef SESSION_H
#define SESSION_H

#include "vi.h"

bool session_save(const char *path);
bool session_load(const char *path);

#endif /* SESSION_H */
