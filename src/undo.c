#include "vi.h"
#include "undo.h"
#include "buffer.h"
#include "screen.h"

static UndoSnapshot take_snapshot(void) {
    UndoSnapshot snap;
    snap.num_lines = E.num_lines;
    if (E.num_lines > 0) {
        snap.lines = (Line *)malloc(sizeof(Line) * E.num_lines);
        for (int i = 0; i < E.num_lines; i++) {
            line_init(&snap.lines[i], E.lines[i].chars, E.lines[i].len);
        }
    } else {
        snap.lines = NULL;
    }
    snap.cx = E.cx;
    snap.cy = E.cy;
    return snap;
}

static void free_snapshot(UndoSnapshot *snap) {
    if (!snap || !snap->lines) return;
    for (int i = 0; i < snap->num_lines; i++) {
        free(snap->lines[i].chars);
    }
    free(snap->lines);
    snap->lines = NULL;
    snap->num_lines = 0;
}

static void push_snapshot(UndoStack *st, UndoSnapshot snap) {
    if (st->top >= MAX_UNDO_LEVELS) {
        free_snapshot(&st->items[0]);
        for (int i = 0; i < MAX_UNDO_LEVELS - 1; i++) {
            st->items[i] = st->items[i + 1];
        }
        st->top = MAX_UNDO_LEVELS - 1;
    }
    st->items[st->top++] = snap;
}

static void clear_stack(UndoStack *st) {
    while (st->top > 0) {
        free_snapshot(&st->items[--st->top]);
    }
}

static void restore_snapshot(UndoSnapshot snap) {
    if (E.lines) {
        for (int i = 0; i < E.num_lines; i++) {
            free(E.lines[i].chars);
        }
        free(E.lines);
    }

    E.lines = snap.lines;
    E.num_lines = snap.num_lines;
    E.cx = snap.cx;
    E.cy = snap.cy;
    E.dirty = true;
}

void clear_undo(void) {
    clear_stack(&E.undo_stack);
    clear_stack(&E.redo_stack);
}

void save_undo(void) {
    if (E.opt.compatible) {
        clear_stack(&E.undo_stack);
    }
    clear_stack(&E.redo_stack);
    push_snapshot(&E.undo_stack, take_snapshot());
}

void apply_undo(void) {
    if (E.undo_stack.top == 0) {
        set_status("Already at oldest change");
        return;
    }

    if (E.opt.compatible) {
        /* Classic 1-level Vi toggle */
        UndoSnapshot cur = take_snapshot();
        UndoSnapshot prev = E.undo_stack.items[--E.undo_stack.top];
        restore_snapshot(prev);
        push_snapshot(&E.undo_stack, cur);
        set_status("1 change undone");
        return;
    }

    /* Modern Vim-style multi-level undo */
    UndoSnapshot cur = take_snapshot();
    push_snapshot(&E.redo_stack, cur);
    UndoSnapshot prev = E.undo_stack.items[--E.undo_stack.top];
    restore_snapshot(prev);
    swap_write();
    set_status("%d change(s) undone; %d left to undo", 1, E.undo_stack.top);
}

void apply_redo(void) {
    if (E.redo_stack.top == 0) {
        set_status("Already at newest change");
        return;
    }

    UndoSnapshot cur = take_snapshot();
    push_snapshot(&E.undo_stack, cur);
    UndoSnapshot next = E.redo_stack.items[--E.redo_stack.top];
    restore_snapshot(next);
    swap_write();
    set_status("%d change(s) redone; %d left to redo", 1, E.redo_stack.top);
}

void backup_u_line(void) {
    if (E.cy >= 0 && E.cy < E.num_lines && E.cy != E.u_line_cy) {
        free(E.u_line_backup);
        E.u_line_backup = _strdup(E.lines[E.cy].chars);
        E.u_line_cy = E.cy;
    }
}

void restore_u_line(void) {
    if (!E.u_line_backup || E.cy < 0 || E.cy >= E.num_lines) return;
    save_undo();
    free(E.lines[E.cy].chars);
    line_init(&E.lines[E.cy], E.u_line_backup, (int)strlen(E.u_line_backup));
    E.dirty = true;
    set_status("Line restored");
}

static void get_undopath(const char *filename, char *out, size_t out_size) {
    const char *sep = strrchr(filename, '\\');
    if (!sep) sep = strrchr(filename, '/');
    if (sep) {
        int dir_len = (int)(sep - filename + 1);
        snprintf(out, out_size, "%.*s.%s.un~", dir_len, filename, sep + 1);
    } else {
        snprintf(out, out_size, ".%s.un~", filename);
    }
}

void undo_save_file(const char *filename) {
    if (!E.opt.undofile || !filename || filename[0] == '[') return;
    char path[MAX_PATH];
    get_undopath(filename, path, sizeof(path));

    FILE *fp = fopen(path, "wb");
    if (!fp) return;

    fprintf(fp, "VIUND10\n%d\n", E.undo_stack.top);
    for (int s = 0; s < E.undo_stack.top; s++) {
        UndoSnapshot *snap = &E.undo_stack.items[s];
        fprintf(fp, "%d %d %d\n", snap->num_lines, snap->cx, snap->cy);
        for (int i = 0; i < snap->num_lines; i++) {
            fprintf(fp, "%d\n", snap->lines[i].len);
            fputs(snap->lines[i].chars, fp);
            fputc('\n', fp);
        }
    }
    fclose(fp);
    SetFileAttributesA(path, FILE_ATTRIBUTE_HIDDEN);
}

void undo_load_file(const char *filename) {
    if (!E.opt.undofile || !filename || filename[0] == '[') return;
    char path[MAX_PATH];
    get_undopath(filename, path, sizeof(path));

    FILE *fp = fopen(path, "rb");
    if (!fp) return;

    char magic[32] = {0};
    if (!fgets(magic, sizeof(magic), fp) || strncmp(magic, "VIUND10", 7) != 0) {
        fclose(fp);
        return;
    }

    int num_snaps = 0;
    if (fscanf(fp, "%d\n", &num_snaps) != 1 || num_snaps <= 0) {
        fclose(fp);
        return;
    }

    if (num_snaps > MAX_UNDO_LEVELS) num_snaps = MAX_UNDO_LEVELS;

    for (int s = 0; s < num_snaps; s++) {
        UndoSnapshot snap;
        if (fscanf(fp, "%d %d %d\n", &snap.num_lines, &snap.cx, &snap.cy) != 3) break;
        snap.lines = (Line *)malloc(sizeof(Line) * snap.num_lines);
        for (int i = 0; i < snap.num_lines; i++) {
            int len = 0;
            if (fscanf(fp, "%d\n", &len) != 1) len = 0;
            char *buf = (char *)malloc(len + 32);
            if (len > 0) {
                if (fgets(buf, len + 31, fp) == NULL) buf[0] = '\0';
                int actual = (int)strlen(buf);
                while (actual > 0 && (buf[actual - 1] == '\n' || buf[actual - 1] == '\r')) buf[--actual] = '\0';
                line_init(&snap.lines[i], buf, actual);
            } else {
                line_init(&snap.lines[i], "", 0);
                char dummy[16];
                fgets(dummy, sizeof(dummy), fp);
            }
            free(buf);
        }
        push_snapshot(&E.undo_stack, snap);
    }
    fclose(fp);
}
