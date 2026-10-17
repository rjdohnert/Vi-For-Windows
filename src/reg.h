#ifndef REG_H
#define REG_H

#include "vi.h"

void reg_free(Register *r);
void reg_set(char reg_name, const char *data, int len, RegType type);
void push_numbered_reg(const char *data, int len);
Register *reg_get(char reg_name);
void do_paste(bool after, int count);

#endif /* REG_H */
