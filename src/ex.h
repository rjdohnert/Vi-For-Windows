#ifndef EX_H
#define EX_H

#include "vi.h"

bool parse_ex_address(const char **s, int *addr);
void parse_ex_range(const char **s, int *start, int *end, bool *has_range);
void execute_substitute(int start_row, int end_row, const char *args);
void execute_set_command(const char *args);
void execute_ex_command(const char *cmd);
void run_ex_mode(void);
void load_virc(void);

#endif /* EX_H */
