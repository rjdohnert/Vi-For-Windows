#ifndef QUICKFIX_H
#define QUICKFIX_H

#include "vi.h"

void quickfix_init(void);
void quickfix_clear(void);
void quickfix_add(const char *filename, int line, int col, const char *text);
bool quickfix_run_grep(const char *pattern);
bool quickfix_run_grep_async(const char *pattern);
bool quickfix_is_running(void);
void quickfix_cancel(void);
void quickfix_wait(DWORD timeout_ms);
int quickfix_get_async_progress(char *out_pattern, int max_pat);
bool quickfix_check_completed(void);
void quickfix_open_window(int height);
void quickfix_close_window(void);
bool quickfix_jump(int index);
bool quickfix_next(void);
bool quickfix_prev(void);
void quickfix_list(void);
bool quickfix_handle_enter(void);

#endif /* QUICKFIX_H */
