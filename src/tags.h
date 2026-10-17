#ifndef TAGS_H
#define TAGS_H

#include "vi.h"

bool tags_jump(const char *symbol);
bool tags_pop(void);
void tags_list(void);

#endif /* TAGS_H */
