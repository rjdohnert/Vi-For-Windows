#include "vi.h"
#include "buffer.h"
#include "screen.h"
#include "undo.h"
#include "syntax.h"
#include "reg.h"
#include "quickfix.h"

void line_init(Line *line, const char *s, int len) {
    line->cap = len + 16;
    line->chars = (char *)malloc(line->cap);
    memcpy(line->chars, s, len);
    line->chars[len] = '\0';
    line->len = len;
}

void line_insert_char(Line *line, int at, char c) {
    if (at < 0 || at > line->len) at = line->len;
    if (line->len + 2 > line->cap) {
        line->cap = (line->cap + 16) * 2;
        line->chars = (char *)realloc(line->chars, line->cap);
    }
    memmove(&line->chars[at + 1], &line->chars[at], line->len - at + 1);
    line->chars[at] = c;
    line->len++;
    E.dirty = true;
}

void line_delete_char(Line *line, int at) {
    if (at < 0 || at >= line->len) return;
    memmove(&line->chars[at], &line->chars[at + 1], line->len - at);
    line->len--;
    E.dirty = true;
}

void line_insert_str(Line *line, int at, const char *str, int slen) {
    if (at < 0 || at > line->len) at = line->len;
    if (line->len + slen + 2 > line->cap) {
        line->cap = (line->cap + slen + 32) * 2;
        line->chars = (char *)realloc(line->chars, line->cap);
    }
    memmove(&line->chars[at + slen], &line->chars[at], line->len - at + 1);
    memcpy(&line->chars[at], str, slen);
    line->len += slen;
    line->chars[line->len] = '\0';
    E.dirty = true;
}

void line_delete_range(Line *line, int at, int count) {
    if (at < 0 || at >= line->len || count <= 0) return;
    if (at + count > line->len) count = line->len - at;
    memmove(&line->chars[at], &line->chars[at + count], line->len - (at + count) + 1);
    line->len -= count;
    E.dirty = true;
}

void insert_row(int at, const char *s, int len) {
    if (at < 0 || at > E.num_lines) return;
    E.lines = (Line *)realloc(E.lines, sizeof(Line) * (E.num_lines + 1));
    memmove(&E.lines[at + 1], &E.lines[at], sizeof(Line) * (E.num_lines - at));
    line_init(&E.lines[at], s, len);
    E.num_lines++;
    E.dirty = true;
}

void delete_row(int at) {
    if (at < 0 || at >= E.num_lines) return;
    free(E.lines[at].chars);
    memmove(&E.lines[at], &E.lines[at + 1], sizeof(Line) * (E.num_lines - at - 1));
    E.num_lines--;
    if (E.num_lines == 0) {
        free(E.lines);
        E.lines = NULL;
    }
    E.dirty = true;
}

void join_two_rows(int row) {
    if (row < 0 || row >= E.num_lines - 1) return;
    Line *l1 = &E.lines[row];
    Line *l2 = &E.lines[row + 1];

    /* Trim trailing spaces from l1 */
    while (l1->len > 0 && isspace((unsigned char)l1->chars[l1->len - 1])) {
        l1->len--;
    }
    l1->chars[l1->len] = '\0';

    /* Skip leading spaces on l2 */
    int l2_start = 0;
    while (l2_start < l2->len && isspace((unsigned char)l2->chars[l2_start])) {
        l2_start++;
    }

    int add_spaces = 0;
    if (l1->len > 0 && l2_start < l2->len) {
        char last = l1->chars[l1->len - 1];
        if (last == '.' || last == '!' || last == '?') {
            add_spaces = 2; /* 2 spaces after sentence endings */
        } else {
            add_spaces = 1;
        }
    }

    int l2_rem = l2->len - l2_start;
    int join_col = l1->len;

    if (add_spaces > 0) {
        for (int i = 0; i < add_spaces; i++) {
            line_insert_char(l1, l1->len, ' ');
        }
    }
    line_insert_str(l1, l1->len, &l2->chars[l2_start], l2_rem);
    delete_row(row + 1);

    E.cy = row;
    E.cx = join_col;
}

/* --- Crash Recovery & Swap File Support --- */

static void get_swap_path(const char *filename, char *out, size_t out_size) {
    if (!filename || filename[0] == '\0') {
        snprintf(out, out_size, ".vi_unnamed.swp");
        return;
    }
    const char *sep = strrchr(filename, '\\');
    const char *sep2 = strrchr(filename, '/');
    if (sep2 && (!sep || sep2 > sep)) sep = sep2;

    if (sep) {
        int dir_len = (int)(sep - filename + 1);
        snprintf(out, out_size, "%.*s.%s.swp", dir_len, filename, sep + 1);
    } else {
        snprintf(out, out_size, ".%s.swp", filename);
    }
}

void swap_write(void) {
    if (E.swap_filename[0] == '\0') return;
    FILE *fp = fopen(E.swap_filename, "wb");
    if (!fp) return;

    fprintf(fp, "VISWP10\n%d\n%d\n", E.is_crlf ? 1 : 0, E.num_lines);
    for (int i = 0; i < E.num_lines; i++) {
        fputs(E.lines[i].chars, fp);
        fputc('\n', fp);
    }
    fclose(fp);
    SetFileAttributesA(E.swap_filename, FILE_ATTRIBUTE_HIDDEN);
}

void swap_unlink(void) {
    if (E.swap_filename[0] != '\0') {
        SetFileAttributesA(E.swap_filename, FILE_ATTRIBUTE_NORMAL);
        remove(E.swap_filename);
        E.swap_filename[0] = '\0';
    }
}

void swap_init(void) {
    get_swap_path(E.filename, E.swap_filename, sizeof(E.swap_filename));
    FILE *fp = fopen(E.swap_filename, "rb");
    if (fp) {
        fclose(fp);
        set_status("ATTENTION: Found swap file for \"%s\"! Use :recover to restore.",
                   E.filename ? E.filename : "[No Name]");
    } else {
        swap_write();
    }
}

bool swap_recover(void) {
    if (E.swap_filename[0] == '\0') {
        get_swap_path(E.filename, E.swap_filename, sizeof(E.swap_filename));
    }
    FILE *fp = fopen(E.swap_filename, "rb");
    if (!fp) {
        set_status("E305: No swap file found for %s", E.filename ? E.filename : "[No Name]");
        return false;
    }

    char magic[32] = {0};
    if (!fgets(magic, sizeof(magic), fp) || strncmp(magic, "VISWP10", 7) != 0) {
        fclose(fp);
        set_status("E306: Invalid swap file header");
        return false;
    }

    int crlf_flag = 0;
    int expected_lines = 0;
    if (fscanf(fp, "%d\n%d\n", &crlf_flag, &expected_lines) != 2) {
        fclose(fp);
        set_status("E306: Invalid swap file format");
        return false;
    }

    save_undo();

    while (E.num_lines > 0) {
        delete_row(0);
    }

    char buf[8192];
    while (fgets(buf, sizeof(buf), fp)) {
        int len = (int)strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) len--;
        insert_row(E.num_lines, buf, len);
    }
    fclose(fp);

    if (E.num_lines == 0) insert_row(0, "", 0);

    E.is_crlf = (crlf_flag != 0);
    E.dirty = true;
    E.cx = 0;
    E.cy = 0;
    set_status("Recovery completed (%d lines). Please review and save with :w", E.num_lines);
    return true;
}

void swap_cleanup_all(void) {
    swap_unlink();
    for (int i = 0; i < E.num_buffers; i++) {
        if (E.buffers[i].swap_filename[0]) {
            SetFileAttributesA(E.buffers[i].swap_filename, FILE_ATTRIBUTE_NORMAL);
            remove(E.buffers[i].swap_filename);
            E.buffers[i].swap_filename[0] = '\0';
        }
    }
}

/* --- Multi-Buffer Operations --- */

static void buffer_load_from_disk(Buffer *b) {
    if (!b || b->loaded) return;
    if (!b->filename) {
        b->lines = (Line *)malloc(sizeof(Line));
        line_init(&b->lines[0], "", 0);
        b->num_lines = 1;
        b->loaded = true;
        return;
    }

    FILE *fp = fopen(b->filename, "rb");
    if (!fp) {
        b->lines = (Line *)malloc(sizeof(Line));
        line_init(&b->lines[0], "", 0);
        b->num_lines = 1;
        b->loaded = true;
        return;
    }

    b->is_crlf = false;
    b->lines = NULL;
    b->num_lines = 0;
    int line_cap = 0;
    char buf[8192];
    while (fgets(buf, sizeof(buf), fp)) {
        int len = (int)strlen(buf);
        if (len >= 2 && buf[len - 2] == '\r' && buf[len - 1] == '\n') {
            b->is_crlf = true;
        }
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) len--;
        if (b->num_lines >= line_cap) {
            line_cap = (line_cap == 0) ? 256 : line_cap * 2;
            b->lines = (Line *)realloc(b->lines, sizeof(Line) * line_cap);
        }
        line_init(&b->lines[b->num_lines], buf, len);
        b->num_lines++;
    }
    fclose(fp);

    if (b->num_lines == 0) {
        b->lines = (Line *)malloc(sizeof(Line));
        line_init(&b->lines[0], "", 0);
        b->num_lines = 1;
    } else if (line_cap > b->num_lines) {
        b->lines = (Line *)realloc(b->lines, sizeof(Line) * b->num_lines);
    }
    b->loaded = true;

    DWORD attr = GetFileAttributesA(b->filename);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY)) {
        b->readonly = true;
    }

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExA(b->filename, GetFileExInfoStandard, &fad)) {
        b->mod_time = fad.ftLastWriteTime;
    }
}

void buffer_init(void) {
    memset(E.buffers, 0, sizeof(E.buffers));
    E.num_buffers = 0;
    E.cur_buf_idx = -1;
    E.next_buffer_id = 1;
    E.swap_filename[0] = '\0';
    memset(&E.comp, 0, sizeof(CompletionState));
}

bool buffer_add(const char *filename) {
    if (!filename || *filename == '\0') return false;
    for (int i = 0; i < E.num_buffers; i++) {
        if (E.buffers[i].filename && strcmp(E.buffers[i].filename, filename) == 0) {
            return true;
        }
    }
    if (E.num_buffers >= MAX_BUFFERS) {
        set_status("E92: Too many buffers");
        return false;
    }
    int idx = E.num_buffers++;
    Buffer *b = &E.buffers[idx];
    memset(b, 0, sizeof(Buffer));
    b->id = E.next_buffer_id++;
    b->filename = _strdup(filename);
    b->loaded = false;
    get_swap_path(b->filename, b->swap_filename, sizeof(b->swap_filename));
    set_status("Added buffer %d: %s", b->id, filename);
    return true;
}

void buffer_save_current_state(void) {
    if (E.cur_buf_idx < 0 || E.cur_buf_idx >= E.num_buffers) return;
    Buffer *b = &E.buffers[E.cur_buf_idx];

    b->lines = E.lines;
    b->num_lines = E.num_lines;
    b->dirty = E.dirty;
    b->is_crlf = E.is_crlf;
    b->readonly = E.readonly;
    b->cx = E.cx;
    b->cy = E.cy;
    b->screen_x = E.screen_x;
    b->row_off = E.row_off;
    b->col_off = E.col_off;
    b->undo_stack = E.undo_stack;
    b->redo_stack = E.redo_stack;
    b->u_line_backup = E.u_line_backup;
    b->u_line_cy = E.u_line_cy;
    memcpy(b->marks, E.marks, sizeof(E.marks));
    strncpy(b->swap_filename, E.swap_filename, sizeof(b->swap_filename) - 1);
    b->swap_filename[sizeof(b->swap_filename) - 1] = '\0';
    b->loaded = true;

    if (b->filename != E.filename) {
        free(b->filename);
        b->filename = E.filename ? _strdup(E.filename) : NULL;
    }
    if (E.cur_win_idx >= 0 && E.cur_win_idx < E.num_windows) {
        E.windows[E.cur_win_idx].buf_idx = E.cur_buf_idx;
    }
}

void buffer_load_state(int idx) {
    if (idx < 0 || idx >= E.num_buffers) return;
    Buffer *b = &E.buffers[idx];
    if (!b->loaded) {
        buffer_load_from_disk(b);
    }

    E.cur_buf_idx = idx;
    if (E.cur_win_idx >= 0 && E.cur_win_idx < E.num_windows) {
        E.windows[E.cur_win_idx].buf_idx = idx;
    }
    E.lines = b->lines;
    E.num_lines = b->num_lines;
    E.dirty = b->dirty;
    E.is_crlf = b->is_crlf;
    E.readonly = b->readonly;
    E.cx = b->cx;
    E.cy = b->cy;
    E.screen_x = b->screen_x;
    E.row_off = b->row_off;
    E.col_off = b->col_off;
    E.undo_stack = b->undo_stack;
    E.redo_stack = b->redo_stack;
    E.u_line_backup = b->u_line_backup;
    E.u_line_cy = b->u_line_cy;
    memcpy(E.marks, b->marks, sizeof(b->marks));

    free(E.filename);
    E.filename = b->filename ? _strdup(b->filename) : NULL;

    if (b->swap_filename[0] == '\0' && E.filename) {
        get_swap_path(E.filename, b->swap_filename, sizeof(b->swap_filename));
    }
    strncpy(E.swap_filename, b->swap_filename, sizeof(E.swap_filename) - 1);
    E.swap_filename[sizeof(E.swap_filename) - 1] = '\0';

    syntax_detect_filetype();
}

bool buffer_switch_to(int idx) {
    if (idx < 0 || idx >= E.num_buffers) {
        set_status("E86: Buffer %d does not exist", idx + 1);
        return false;
    }
    if (idx == E.cur_buf_idx) return true;

    buffer_save_current_state();
    buffer_load_state(idx);
    if (E.cur_win_idx >= 0 && E.cur_win_idx < E.num_windows) {
        E.windows[E.cur_win_idx].buf_idx = idx;
    }
    set_status("\"%s\" %dL", E.filename ? E.filename : "[No Name]", E.num_lines);
    return true;
}

bool buffer_switch_by_name(const char *name) {
    while (*name == ' ') name++;
    if (*name == '\0') {
        buffer_list();
        return true;
    }

    if (isdigit((unsigned char)*name)) {
        int target = atoi(name);
        for (int i = 0; i < E.num_buffers; i++) {
            if (E.buffers[i].id == target) {
                return buffer_switch_to(i);
            }
        }
        if (target >= 1 && target <= E.num_buffers) {
            return buffer_switch_to(target - 1);
        }
    }

    for (int i = 0; i < E.num_buffers; i++) {
        if (E.buffers[i].filename && _stricmp(E.buffers[i].filename, name) == 0) {
            return buffer_switch_to(i);
        }
    }

    for (int i = 0; i < E.num_buffers; i++) {
        if (E.buffers[i].filename && strstr(E.buffers[i].filename, name) != NULL) {
            return buffer_switch_to(i);
        }
    }

    set_status("E94: No matching buffer for %s", name);
    return false;
}

void buffer_next(void) {
    if (E.num_buffers <= 1) {
        set_status("Only one buffer");
        return;
    }
    int next = (E.cur_buf_idx + 1) % E.num_buffers;
    buffer_switch_to(next);
}

void buffer_prev(void) {
    if (E.num_buffers <= 1) {
        set_status("Only one buffer");
        return;
    }
    int prev = (E.cur_buf_idx - 1 + E.num_buffers) % E.num_buffers;
    buffer_switch_to(prev);
}

bool buffer_delete(int idx, bool force) {
    if (idx < 0 || idx >= E.num_buffers) {
        set_status("E86: Buffer does not exist");
        return false;
    }

    Buffer *b = &E.buffers[idx];
    bool is_dirty = (idx == E.cur_buf_idx) ? E.dirty : b->dirty;
    if (is_dirty && !force) {
        set_status("E89: No write since last change for buffer %d (add ! to override)", b->id);
        return false;
    }

    if (idx == E.cur_buf_idx) {
        swap_unlink();
    } else if (b->swap_filename[0]) {
        SetFileAttributesA(b->swap_filename, FILE_ATTRIBUTE_NORMAL);
        remove(b->swap_filename);
    }

    Line *lines = (idx == E.cur_buf_idx) ? E.lines : b->lines;
    int num_lines = (idx == E.cur_buf_idx) ? E.num_lines : b->num_lines;
    if (lines) {
        for (int i = 0; i < num_lines; i++) {
            free(lines[i].chars);
        }
        free(lines);
    }

    UndoStack *us = (idx == E.cur_buf_idx) ? &E.undo_stack : &b->undo_stack;
    for (int i = 0; i < us->top; i++) {
        for (int j = 0; j < us->items[i].num_lines; j++) {
            free(us->items[i].lines[j].chars);
        }
        free(us->items[i].lines);
    }
    UndoStack *rs = (idx == E.cur_buf_idx) ? &E.redo_stack : &b->redo_stack;
    for (int i = 0; i < rs->top; i++) {
        for (int j = 0; j < rs->items[i].num_lines; j++) {
            free(rs->items[i].lines[j].chars);
        }
        free(rs->items[i].lines);
    }
    if (idx == E.cur_buf_idx) {
        free(E.u_line_backup);
        E.u_line_backup = NULL;
    } else {
        free(b->u_line_backup);
        b->u_line_backup = NULL;
    }

    free(b->filename);
    b->filename = NULL;

    if (E.num_buffers == 1) {
        E.lines = (Line *)malloc(sizeof(Line));
        line_init(&E.lines[0], "", 0);
        E.num_lines = 1;
        E.cx = 0;
        E.cy = 0;
        E.screen_x = 0;
        E.row_off = 0;
        E.col_off = 0;
        free(E.filename);
        E.filename = NULL;
        E.dirty = false;
        E.is_crlf = false;
        E.undo_stack.top = 0;
        E.redo_stack.top = 0;
        E.u_line_backup = NULL;
        E.u_line_cy = -1;
        memset(E.marks, 0, sizeof(E.marks));
        E.swap_filename[0] = '\0';

        E.buffers[0].id = E.next_buffer_id++;
        E.buffers[0].filename = NULL;
        E.buffers[0].loaded = true;
        buffer_save_current_state();
        set_status("Buffer deleted, [No Name] buffer created");
        return true;
    }

    for (int i = idx; i < E.num_buffers - 1; i++) {
        E.buffers[i] = E.buffers[i + 1];
    }
    E.num_buffers--;

    int fallback_idx = (idx >= E.num_buffers) ? E.num_buffers - 1 : idx;
    for (int w = 0; w < E.num_windows; w++) {
        if (E.windows[w].buf_idx == idx) E.windows[w].buf_idx = fallback_idx;
        else if (E.windows[w].buf_idx > idx) E.windows[w].buf_idx--;
    }
    for (int t = 0; t < E.num_tabs; t++) {
        for (int w = 0; w < E.tabs[t].num_windows; w++) {
            if (E.tabs[t].windows[w].buf_idx == idx) E.tabs[t].windows[w].buf_idx = fallback_idx;
            else if (E.tabs[t].windows[w].buf_idx > idx) E.tabs[t].windows[w].buf_idx--;
        }
    }

    if (idx == E.cur_buf_idx) {
        E.cur_buf_idx = -1;
        buffer_load_state(fallback_idx);
    } else if (E.cur_buf_idx > idx) {
        E.cur_buf_idx--;
    }

    set_status("Buffer deleted");
    return true;
}

void buffer_list(void) {
    if (E.num_buffers == 0) {
        set_status("No buffers");
        return;
    }
    buffer_save_current_state();

    char out[256] = {0};
    int pos = 0;
    for (int i = 0; i < E.num_buffers && pos < (int)sizeof(out) - 20; i++) {
        Buffer *b = &E.buffers[i];
        char cur = (i == E.cur_buf_idx) ? '%' : ' ';
        char dirty = b->dirty ? '+' : (b->loaded ? 'a' : ' ');
        const char *name = b->filename ? b->filename : "[No Name]";
        const char *slash = strrchr(name, '\\');
        if (!slash) slash = strrchr(name, '/');
        const char *disp = slash ? slash + 1 : name;

        int written = snprintf(&out[pos], sizeof(out) - pos, "%d%c%c \"%s\"%s",
                               b->id, cur, dirty, disp,
                               (i < E.num_buffers - 1) ? "  " : "");
        if (written > 0) pos += written;
    }
    set_status("%s", out);
}

/* --- Buffer-Word Autocompletion --- */

void completion_reset(void) {
    E.comp.active = false;
    E.comp.count = 0;
    E.comp.current_index = -1;
}

static bool is_word_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static void add_completion_candidate(const char *w, int wlen, const char *prefix, int prefix_len) {
    if (wlen <= prefix_len || wlen >= 128) return;
    bool match = E.opt.ignorecase ? (_strnicmp(w, prefix, prefix_len) == 0)
                                  : (strncmp(w, prefix, prefix_len) == 0);
    if (!match) return;

    char temp[128];
    memcpy(temp, w, wlen);
    temp[wlen] = '\0';

    for (int i = 0; i < E.comp.count; i++) {
        if (strcmp(E.comp.items[i], temp) == 0) return;
    }
    if (E.comp.count < MAX_COMPLETIONS) {
        strcpy(E.comp.items[E.comp.count++], temp);
    }
}

static void scan_line_for_words(const char *chars, int len, const char *prefix, int prefix_len) {
    int i = 0;
    while (i < len && E.comp.count < MAX_COMPLETIONS) {
        while (i < len && !is_word_char(chars[i])) i++;
        int wstart = i;
        while (i < len && is_word_char(chars[i])) i++;
        int wlen = i - wstart;
        if (wlen > 0) {
            add_completion_candidate(&chars[wstart], wlen, prefix, prefix_len);
        }
    }
}

void completion_trigger(int dir) {
    if (E.mode != MODE_INSERT && E.mode != MODE_REPLACE) return;
    if (E.cy < 0 || E.cy >= E.num_lines) return;

    Line *cur_line = &E.lines[E.cy];

    if (E.comp.active && E.comp.count > 0) {
        if (dir > 0) {
            E.comp.current_index = (E.comp.current_index + 1) % E.comp.count;
        } else {
            E.comp.current_index = (E.comp.current_index - 1 + E.comp.count) % E.comp.count;
        }

        int cur_len = E.cx - E.comp.start_col;
        if (cur_len > 0) {
            line_delete_range(cur_line, E.comp.start_col, cur_len);
        }
        const char *replacement = E.comp.items[E.comp.current_index];
        int rlen = (int)strlen(replacement);
        line_insert_str(cur_line, E.comp.start_col, replacement, rlen);
        E.cx = E.comp.start_col + rlen;
        set_status("-- Keyword completion (^N^P) match %d of %d --",
                   E.comp.current_index + 1, E.comp.count);
        return;
    }

    int end_col = E.cx;
    int start_col = end_col;
    while (start_col > 0 && is_word_char(cur_line->chars[start_col - 1])) {
        start_col--;
    }
    int prefix_len = end_col - start_col;
    if (prefix_len <= 0) {
        set_status("-- No prefix for completion --");
        return;
    }

    char prefix[128] = {0};
    if (prefix_len >= (int)sizeof(prefix)) prefix_len = (int)sizeof(prefix) - 1;
    strncpy(prefix, &cur_line->chars[start_col], prefix_len);
    prefix[prefix_len] = '\0';

    E.comp.count = 0;
    E.comp.current_index = 0;
    E.comp.start_col = start_col;
    E.comp.prefix_len = prefix_len;

    /* 1. Scan current buffer lines (prioritizing proximity to cursor) */
    for (int r = E.cy; r >= 0 && E.comp.count < MAX_COMPLETIONS; r--) {
        scan_line_for_words(E.lines[r].chars, E.lines[r].len, prefix, prefix_len);
    }
    for (int r = E.cy + 1; r < E.num_lines && E.comp.count < MAX_COMPLETIONS; r++) {
        scan_line_for_words(E.lines[r].chars, E.lines[r].len, prefix, prefix_len);
    }

    /* 2. Scan other loaded buffers */
    for (int b = 0; b < E.num_buffers && E.comp.count < MAX_COMPLETIONS; b++) {
        if (b == E.cur_buf_idx) continue;
        Buffer *buf = &E.buffers[b];
        if (!buf->loaded || !buf->lines) continue;
        for (int r = 0; r < buf->num_lines && E.comp.count < MAX_COMPLETIONS; r++) {
            scan_line_for_words(buf->lines[r].chars, buf->lines[r].len, prefix, prefix_len);
        }
    }

    if (E.comp.count == 0) {
        set_status("-- Pattern not found --");
        return;
    }

    E.comp.active = true;
    E.comp.current_index = (dir > 0) ? 0 : (E.comp.count - 1);

    int cur_len = E.cx - E.comp.start_col;
    if (cur_len > 0) {
        line_delete_range(cur_line, E.comp.start_col, cur_len);
    }
    const char *replacement = E.comp.items[E.comp.current_index];
    int rlen = (int)strlen(replacement);
    line_insert_str(cur_line, E.comp.start_col, replacement, rlen);
    E.cx = E.comp.start_col + rlen;
    set_status("-- Keyword completion (^N^P) match %d of %d --",
               E.comp.current_index + 1, E.comp.count);
}

/* --- Editor Open & Save --- */

void editor_open(const char *filename) {
    if (!filename) return;

    for (int i = 0; i < E.num_buffers; i++) {
        if (E.buffers[i].filename && strcmp(E.buffers[i].filename, filename) == 0) {
            buffer_switch_to(i);
            return;
        }
    }

    if (E.cur_buf_idx >= 0 && E.cur_buf_idx < E.num_buffers) {
        Buffer *cur_b = &E.buffers[E.cur_buf_idx];
        if (!E.dirty && cur_b->filename == NULL && E.num_lines == 1 && E.lines[0].len == 0) {
            free(E.filename);
            E.filename = _strdup(filename);
            free(cur_b->filename);
            cur_b->filename = _strdup(filename);
            delete_row(0);
        } else {
            if (E.num_buffers >= MAX_BUFFERS) {
                set_status("E92: Too many buffers");
                return;
            }
            buffer_save_current_state();
            int new_idx = E.num_buffers++;
            Buffer *nb = &E.buffers[new_idx];
            memset(nb, 0, sizeof(Buffer));
            nb->id = E.next_buffer_id++;
            nb->filename = _strdup(filename);
            nb->loaded = true;
            E.cur_buf_idx = new_idx;
            E.lines = NULL;
            E.num_lines = 0;
            E.undo_stack.top = 0;
            E.redo_stack.top = 0;
            E.u_line_backup = NULL;
            E.u_line_cy = -1;
            memset(E.marks, 0, sizeof(E.marks));
            E.cx = 0;
            E.cy = 0;
            E.row_off = 0;
            E.col_off = 0;
            free(E.filename);
            E.filename = _strdup(filename);
        }
    } else {
        int idx = 0;
        E.num_buffers = 1;
        E.cur_buf_idx = 0;
        Buffer *nb = &E.buffers[idx];
        memset(nb, 0, sizeof(Buffer));
        nb->id = E.next_buffer_id++;
        nb->filename = _strdup(filename);
        nb->loaded = true;
        free(E.filename);
        E.filename = _strdup(filename);
        if (E.lines) {
            for (int i = 0; i < E.num_lines; i++) free(E.lines[i].chars);
            free(E.lines);
        }
        E.lines = NULL;
        E.num_lines = 0;
    }

    syntax_detect_filetype();

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        insert_row(0, "", 0);
        E.dirty = false;
        swap_init();
        set_status("\"%s\" [New File]", filename);
        buffer_save_current_state();
        return;
    }

    E.is_crlf = false;
    char buf[8192];
    int byte_count = 0;
    int line_cap = 0;
    while (fgets(buf, sizeof(buf), fp)) {
        int len = (int)strlen(buf);
        byte_count += len;
        if (len >= 2 && buf[len - 2] == '\r' && buf[len - 1] == '\n') {
            E.is_crlf = true;
        }
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) len--;
        if (E.num_lines >= line_cap) {
            line_cap = (line_cap == 0) ? 256 : line_cap * 2;
            E.lines = (Line *)realloc(E.lines, sizeof(Line) * line_cap);
        }
        line_init(&E.lines[E.num_lines], buf, len);
        E.num_lines++;
    }
    fclose(fp);

    if (E.num_lines == 0) {
        insert_row(0, "", 0);
    } else if (line_cap > E.num_lines) {
        E.lines = (Line *)realloc(E.lines, sizeof(Line) * E.num_lines);
    }

    DWORD attr = GetFileAttributesA(filename);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY)) {
        E.readonly = true;
    }

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExA(filename, GetFileExInfoStandard, &fad)) {
        if (E.cur_buf_idx >= 0 && E.cur_buf_idx < E.num_buffers) {
            E.buffers[E.cur_buf_idx].mod_time = fad.ftLastWriteTime;
        }
    }

    E.dirty = false;
    swap_init();
    undo_load_file(filename);
    set_status("\"%s\"%s%s %dL, %dC", filename, E.readonly ? " [readonly]" : "", E.is_crlf ? " [dos]" : "", E.num_lines, byte_count);
    backup_u_line();
    buffer_save_current_state();
}

bool editor_save(const char *filename, bool force) {
    const char *target = filename ? filename : E.filename;
    if (!target) {
        set_status("E32: No file name");
        return false;
    }

    if (E.readonly && !force) {
        set_status("E45: 'readonly' option is set (add ! to override)");
        return false;
    }

    DWORD attr = GetFileAttributesA(target);
    bool cleared_ro = false;
    if (force && attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY)) {
        SetFileAttributesA(target, attr & ~FILE_ATTRIBUTE_READONLY);
        cleared_ro = true;
    }

    FILE *fp = fopen(target, "wb");
    if (!fp) {
        if (cleared_ro) SetFileAttributesA(target, attr);
        set_status("E212: Cannot open file for writing (access denied or readonly)");
        return false;
    }

    int bytes = 0;
    const char *eol = E.is_crlf ? "\r\n" : "\n";
    int eol_len = (int)strlen(eol);
    for (int i = 0; i < E.num_lines; i++) {
        fputs(E.lines[i].chars, fp);
        fputs(eol, fp);
        bytes += E.lines[i].len + eol_len;
    }
    fclose(fp);

    if (cleared_ro) {
        SetFileAttributesA(target, attr);
    }

    if (!E.filename || strcmp(E.filename, target) != 0) {
        free(E.filename);
        E.filename = _strdup(target);
        if (E.cur_buf_idx >= 0 && E.cur_buf_idx < E.num_buffers) {
            free(E.buffers[E.cur_buf_idx].filename);
            E.buffers[E.cur_buf_idx].filename = _strdup(target);
        }
        swap_init();
    }

    E.dirty = false;
    if (E.cur_buf_idx >= 0 && E.cur_buf_idx < E.num_buffers) {
        E.buffers[E.cur_buf_idx].dirty = false;
    }

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExA(target, GetFileExInfoStandard, &fad)) {
        if (E.cur_buf_idx >= 0 && E.cur_buf_idx < E.num_buffers) {
            E.buffers[E.cur_buf_idx].mod_time = fad.ftLastWriteTime;
        }
    }

    swap_write();
    undo_save_file(target);
    set_status("\"%s\"%s %dL, %dC written", target, E.is_crlf ? " [dos]" : "", E.num_lines, bytes);
    return true;
}

void editor_cleanup(void) {
    quickfix_cancel();
    swap_cleanup_all();

    for (int b = 0; b < E.num_buffers; b++) {
        Buffer *buf = &E.buffers[b];
        Line *lines = (b == E.cur_buf_idx) ? E.lines : buf->lines;
        int num_lines = (b == E.cur_buf_idx) ? E.num_lines : buf->num_lines;
        if (lines) {
            for (int i = 0; i < num_lines; i++) {
                free(lines[i].chars);
            }
            free(lines);
            if (b == E.cur_buf_idx) E.lines = NULL;
            buf->lines = NULL;
        }
        free(buf->filename);
        buf->filename = NULL;

        UndoStack *us = (b == E.cur_buf_idx) ? &E.undo_stack : &buf->undo_stack;
        for (int i = 0; i < us->top; i++) {
            for (int j = 0; j < us->items[i].num_lines; j++) {
                free(us->items[i].lines[j].chars);
            }
            free(us->items[i].lines);
        }
        us->top = 0;

        UndoStack *rs = (b == E.cur_buf_idx) ? &E.redo_stack : &buf->redo_stack;
        for (int i = 0; i < rs->top; i++) {
            for (int j = 0; j < rs->items[i].num_lines; j++) {
                free(rs->items[i].lines[j].chars);
            }
            free(rs->items[i].lines);
        }
        rs->top = 0;

        if (b == E.cur_buf_idx) {
            free(E.u_line_backup);
            E.u_line_backup = NULL;
        } else {
            free(buf->u_line_backup);
            buf->u_line_backup = NULL;
        }
    }
    E.num_buffers = 0;

    if (E.lines) {
        for (int i = 0; i < E.num_lines; i++) {
            free(E.lines[i].chars);
        }
        free(E.lines);
        E.lines = NULL;
    }
    E.num_lines = 0;

    for (int i = 0; i < E.undo_stack.top; i++) {
        for (int j = 0; j < E.undo_stack.items[i].num_lines; j++) {
            free(E.undo_stack.items[i].lines[j].chars);
        }
        free(E.undo_stack.items[i].lines);
    }
    E.undo_stack.top = 0;

    for (int i = 0; i < E.redo_stack.top; i++) {
        for (int j = 0; j < E.redo_stack.items[i].num_lines; j++) {
            free(E.redo_stack.items[i].lines[j].chars);
        }
        free(E.redo_stack.items[i].lines);
    }
    E.redo_stack.top = 0;

    free(E.u_line_backup);
    E.u_line_backup = NULL;

    free(E.filename);
    E.filename = NULL;

    reg_free(&E.reg_unnamed);
    reg_free(&E.reg_clipboard);
    for (int i = 0; i < 26; i++) reg_free(&E.reg_named[i]);
    for (int i = 0; i < 9; i++) reg_free(&E.reg_numbered[i]);

    free(E.last_action.inserted);
    E.last_action.inserted = NULL;
    free(E.curr_action.inserted);
    E.curr_action.inserted = NULL;
}

bool buffer_check_file_modified(Buffer *b) {
    if (!b || !b->filename || !b->loaded) return false;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(b->filename, GetFileExInfoStandard, &fad)) return false;
    if (b->mod_time.dwLowDateTime == 0 && b->mod_time.dwHighDateTime == 0) return false;
    return (CompareFileTime(&fad.ftLastWriteTime, &b->mod_time) > 0);
}

bool buffer_reload(Buffer *b) {
    if (!b || !b->filename) return false;
    FILE *fp = fopen(b->filename, "rb");
    if (!fp) return false;

    bool is_current = (E.cur_buf_idx >= 0 && &E.buffers[E.cur_buf_idx] == b);
    Line *lines = is_current ? E.lines : b->lines;
    int num_lines = is_current ? E.num_lines : b->num_lines;
    if (lines) {
        for (int i = 0; i < num_lines; i++) free(lines[i].chars);
        free(lines);
    }

    Line *new_lines = NULL;
    int new_num = 0;
    int line_cap = 0;
    bool is_crlf = false;
    char buf[8192];

    while (fgets(buf, sizeof(buf), fp)) {
        int len = (int)strlen(buf);
        if (len >= 2 && buf[len - 2] == '\r' && buf[len - 1] == '\n') is_crlf = true;
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) len--;
        if (new_num >= line_cap) {
            line_cap = (line_cap == 0) ? 256 : line_cap * 2;
            new_lines = (Line *)realloc(new_lines, sizeof(Line) * line_cap);
        }
        line_init(&new_lines[new_num], buf, len);
        new_num++;
    }
    fclose(fp);

    if (new_num == 0) {
        new_lines = (Line *)malloc(sizeof(Line));
        line_init(&new_lines[0], "", 0);
        new_num = 1;
    }

    if (is_current) {
        E.lines = new_lines;
        E.num_lines = new_num;
        E.is_crlf = is_crlf;
        E.dirty = false;
        if (E.cy >= E.num_lines) E.cy = E.num_lines - 1;
    }
    b->lines = new_lines;
    b->num_lines = new_num;
    b->is_crlf = is_crlf;
    b->dirty = false;
    if (b->cy >= b->num_lines) b->cy = b->num_lines - 1;

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExA(b->filename, GetFileExInfoStandard, &fad)) {
        b->mod_time = fad.ftLastWriteTime;
    }
    return true;
}

void buffer_check_all_modified(void) {
    int count = 0;
    for (int i = 0; i < E.num_buffers; i++) {
        Buffer *b = &E.buffers[i];
        if (buffer_check_file_modified(b)) {
            count++;
            bool is_dirty = (i == E.cur_buf_idx) ? E.dirty : b->dirty;
            if (E.opt.autoread && !is_dirty) {
                buffer_reload(b);
                set_status("\"%s\" reloaded from disk (modified externally)", b->filename ? b->filename : "[No Name]");
            } else {
                set_status("Warning: File \"%s\" has changed since last read! Use :e! to reload or :w! to overwrite",
                           b->filename ? b->filename : "[No Name]");
            }
        }
    }
    if (count == 0) {
        set_status("All buffers up to date");
    }
}
