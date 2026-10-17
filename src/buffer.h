#ifndef BUFFER_H
#define BUFFER_H

#include "vi.h"

void line_init(Line *line, const char *s, int len);
void line_insert_char(Line *line, int at, char c);
void line_delete_char(Line *line, int at);
void line_insert_str(Line *line, int at, const char *str, int slen);
void line_delete_range(Line *line, int at, int count);
void insert_row(int at, const char *s, int len);
void delete_row(int at);
void join_two_rows(int row);

void editor_open(const char *filename);
bool editor_save(const char *filename, bool force);

/* Multi-Buffer Operations */
void buffer_init(void);
bool buffer_add(const char *filename);
void buffer_save_current_state(void);
void buffer_load_state(int idx);
bool buffer_switch_to(int idx);
bool buffer_switch_by_name(const char *name);
void buffer_next(void);
void buffer_prev(void);
bool buffer_delete(int idx, bool force);
void buffer_list(void);

/* Buffer-Word Autocompletion */
void completion_trigger(int dir);
void completion_reset(void);

/* Crash Recovery & Swap */
void swap_init(void);
void swap_write(void);
void swap_unlink(void);
bool swap_recover(void);
void swap_cleanup_all(void);
void editor_cleanup(void);

/* File Modification Detection */
bool buffer_check_file_modified(Buffer *b);
bool buffer_reload(Buffer *b);
void buffer_check_all_modified(void);

#endif /* BUFFER_H */
