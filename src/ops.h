#ifndef OPS_H
#define OPS_H

#include "vi.h"

void apply_operator_on_range(char op, int y1, int x1, int y2, int x2, bool linewise);
void apply_block_operator(char op);
void record_insert_char(char c);
void finalize_repeat_action(void);
void repeat_last_action(int override_count);

#endif /* OPS_H */
