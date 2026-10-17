#ifndef DIFF_H
#define DIFF_H

#include "vi.h"

typedef enum {
    DIFF_NONE = 0,
    DIFF_SAME,
    DIFF_CHANGE,
    DIFF_ADD,
    DIFF_DEL
} DiffStatus;

void diff_init(void);
void diff_cleanup(void);
void diff_compute(void);
bool diff_split(const char *filename);
void diff_this(void);
void diff_off(void);
DiffStatus diff_get_status(int buf_idx, int line_idx);
void diff_sync_scroll(void);

#endif /* DIFF_H */
