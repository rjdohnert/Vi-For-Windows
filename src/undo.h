#ifndef UNDO_H
#define UNDO_H

void save_undo(void);
void apply_undo(void);
void apply_redo(void);
void clear_undo(void);
void backup_u_line(void);
void restore_u_line(void);
void undo_save_file(const char *filename);
void undo_load_file(const char *filename);

#endif /* UNDO_H */
