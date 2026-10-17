#ifndef SEARCH_H
#define SEARCH_H

#include "vi.h"

const char *regex_search_line(const char *pat, const char *text, bool ic, int *match_len);
void editor_search(bool forward);

#endif /* SEARCH_H */
