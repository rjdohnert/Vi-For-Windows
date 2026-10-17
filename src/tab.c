#include "vi.h"
#include "tab.h"
#include "window.h"
#include "buffer.h"
#include "screen.h"

void tab_init(void) {
    E.num_tabs = 1;
    E.cur_tab_idx = 0;
    E.next_tab_id = 1;
    memset(E.tabs, 0, sizeof(E.tabs));
    E.tabs[0].id = E.next_tab_id++;
    strcpy(E.tabs[0].title, "[No Name]");
}

void tab_save_current(void) {
    if (E.cur_tab_idx < 0 || E.cur_tab_idx >= E.num_tabs) return;
    window_save_active();
    EditorTab *t = &E.tabs[E.cur_tab_idx];
    memcpy(t->windows, E.windows, sizeof(E.windows));
    t->num_windows = E.num_windows;
    t->cur_win_idx = E.cur_win_idx;

    const char *fname = E.filename;
    if (fname) {
        const char *slash = strrchr(fname, '\\');
        if (!slash) slash = strrchr(fname, '/');
        const char *base = slash ? slash + 1 : fname;
        strncpy(t->title, base, sizeof(t->title) - 1);
        t->title[sizeof(t->title) - 1] = '\0';
    } else {
        strcpy(t->title, "[No Name]");
    }
}

void tab_load(int idx) {
    if (idx < 0 || idx >= E.num_tabs) return;
    E.cur_tab_idx = idx;
    EditorTab *t = &E.tabs[idx];
    memcpy(E.windows, t->windows, sizeof(E.windows));
    E.num_windows = t->num_windows;
    E.cur_win_idx = t->cur_win_idx;

    /* Re-adjust window top based on whether tabline is shown */
    int top_offset = (E.num_tabs > 1) ? 1 : 0;
    int avail_rows = E.screen_rows - top_offset;
    if (E.num_windows == 1) {
        E.windows[0].top = top_offset;
        E.windows[0].rows = avail_rows;
        E.windows[0].cols = E.screen_cols;
    }

    window_load_active(E.cur_win_idx);
}

bool tab_new(const char *filename) {
    if (E.num_tabs >= MAX_TABS) {
        set_status("Too many tab pages");
        return false;
    }

    tab_save_current();

    int new_idx = E.num_tabs++;
    EditorTab *t = &E.tabs[new_idx];
    memset(t, 0, sizeof(EditorTab));
    t->id = E.next_tab_id++;

    t->num_windows = 1;
    t->cur_win_idx = 0;
    EditorWindow *w = &t->windows[0];
    w->id = E.next_win_id++;
    w->top = 1;
    w->left = 0;
    w->rows = (E.screen_rows > 1) ? E.screen_rows - 1 : 23;
    w->cols = E.screen_cols;

    tab_load(new_idx);

    if (filename && *filename) {
        editor_open(filename);
    } else {
        /* Open new empty buffer */
        if (E.num_buffers < MAX_BUFFERS) {
            int b_idx = E.num_buffers++;
            Buffer *b = &E.buffers[b_idx];
            memset(b, 0, sizeof(Buffer));
            b->id = E.next_buffer_id++;
            b->filename = NULL;
            b->loaded = true;
            buffer_load_state(b_idx);
            insert_row(0, "", 0);
            E.dirty = false;
        }
    }

    tab_save_current();
    set_status("Tab %d created", t->id);
    return true;
}

bool tab_close(void) {
    if (E.num_tabs <= 1) {
        set_status("Cannot close last tab page");
        return false;
    }

    int closed = E.cur_tab_idx;
    for (int i = closed; i < E.num_tabs - 1; i++) {
        E.tabs[i] = E.tabs[i + 1];
    }
    E.num_tabs--;

    int next = (closed >= E.num_tabs) ? E.num_tabs - 1 : closed;
    tab_load(next);
    set_status("Tab closed");
    return true;
}

void tab_next(void) {
    if (E.num_tabs <= 1) return;
    tab_save_current();
    int next = (E.cur_tab_idx + 1) % E.num_tabs;
    tab_load(next);
}

void tab_prev(void) {
    if (E.num_tabs <= 1) return;
    tab_save_current();
    int prev = (E.cur_tab_idx - 1 + E.num_tabs) % E.num_tabs;
    tab_load(prev);
}

void tab_switch(int idx) {
    if (idx < 0 || idx >= E.num_tabs || idx == E.cur_tab_idx) return;
    tab_save_current();
    tab_load(idx);
}

void tab_only(void) {
    if (E.num_tabs <= 1) return;
    EditorTab cur = E.tabs[E.cur_tab_idx];
    E.tabs[0] = cur;
    E.num_tabs = 1;
    E.cur_tab_idx = 0;
    tab_load(0);
    set_status("Closed all other tabs");
}

bool tab_handle_click(int mouse_x) {
    if (E.num_tabs <= 1) return false;
    int current_col = 1;
    for (int i = 0; i < E.num_tabs; i++) {
        bool is_cur = (i == E.cur_tab_idx);
        const char *title = E.tabs[i].title[0] ? E.tabs[i].title : "[No Name]";
        if (is_cur && E.filename) {
            const char *slash = strrchr(E.filename, '\\');
            if (!slash) slash = strrchr(E.filename, '/');
            title = slash ? slash + 1 : E.filename;
        }
        char tab_text[128];
        int tlen = snprintf(tab_text, sizeof(tab_text), " %d: %s%s ",
                            i + 1, title, (is_cur && E.dirty) ? " +" : "");
        if (mouse_x >= current_col && mouse_x < current_col + tlen) {
            tab_switch(i);
            return true;
        }
        current_col += tlen;
        if (current_col >= E.screen_cols) break;
    }
    return false;
}

