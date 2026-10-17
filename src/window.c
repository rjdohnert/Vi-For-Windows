#include "vi.h"
#include "window.h"
#include "buffer.h"
#include "screen.h"

void window_init(void) {
    E.num_windows = 1;
    E.cur_win_idx = 0;
    E.next_win_id = 1;
    EditorWindow *w = &E.windows[0];
    memset(w, 0, sizeof(EditorWindow));
    w->id = E.next_win_id++;
    w->buf_idx = (E.cur_buf_idx >= 0) ? E.cur_buf_idx : 0;
    w->top = 0;
    w->left = 0;
    w->rows = (E.screen_rows > 0) ? E.screen_rows : 24;
    w->cols = (E.screen_cols > 0) ? E.screen_cols : 80;
    w->cx = E.cx;
    w->cy = E.cy;
    w->screen_x = E.screen_x;
    w->row_off = E.row_off;
    w->col_off = E.col_off;
}

void window_save_active(void) {
    if (E.cur_win_idx < 0 || E.cur_win_idx >= E.num_windows) return;
    EditorWindow *w = &E.windows[E.cur_win_idx];
    w->buf_idx = E.cur_buf_idx;
    w->cx = E.cx;
    w->cy = E.cy;
    w->screen_x = E.screen_x;
    w->row_off = E.row_off;
    w->col_off = E.col_off;
}

void window_load_active(int idx) {
    if (idx < 0 || idx >= E.num_windows) return;
    E.cur_win_idx = idx;
    EditorWindow *w = &E.windows[idx];
    if (w->buf_idx >= 0 && w->buf_idx < E.num_buffers && w->buf_idx != E.cur_buf_idx) {
        buffer_switch_to(w->buf_idx);
    }
    E.cx = w->cx;
    E.cy = w->cy;
    E.screen_x = w->screen_x;
    E.row_off = w->row_off;
    E.col_off = w->col_off;
}

bool window_split(bool vertical) {
    if (E.num_windows >= MAX_WINDOWS) {
        set_status("Too many windows");
        return false;
    }
    window_save_active();
    EditorWindow *cur = &E.windows[E.cur_win_idx];

    if (!vertical) {
        /* Horizontal split */
        if (cur->rows < 4) {
            set_status("Window too small to split horizontally");
            return false;
        }
        int orig_rows = cur->rows;
        int r1 = orig_rows / 2;
        int r2 = orig_rows - r1 - 1; /* 1 row for separator / status */
        if (r2 < 1) r2 = 1;

        cur->rows = r1;

        int new_idx = E.num_windows++;
        EditorWindow *nw = &E.windows[new_idx];
        memset(nw, 0, sizeof(EditorWindow));
        nw->id = E.next_win_id++;
        nw->buf_idx = cur->buf_idx;
        nw->top = cur->top + r1 + 1;
        nw->left = cur->left;
        nw->rows = r2;
        nw->cols = cur->cols;
        nw->cx = cur->cx;
        nw->cy = cur->cy;
        nw->screen_x = cur->screen_x;
        nw->row_off = cur->row_off;
        nw->col_off = cur->col_off;

        window_load_active(new_idx);
        set_status("Split horizontal (Win %d)", nw->id);
        return true;
    } else {
        /* Vertical split */
        if (cur->cols < 10) {
            set_status("Window too small to split vertically");
            return false;
        }
        int orig_cols = cur->cols;
        int c1 = orig_cols / 2;
        int c2 = orig_cols - c1 - 1; /* 1 col for separator */
        if (c2 < 4) c2 = 4;

        cur->cols = c1;

        int new_idx = E.num_windows++;
        EditorWindow *nw = &E.windows[new_idx];
        memset(nw, 0, sizeof(EditorWindow));
        nw->id = E.next_win_id++;
        nw->buf_idx = cur->buf_idx;
        nw->top = cur->top;
        nw->left = cur->left + c1 + 1;
        nw->rows = cur->rows;
        nw->cols = c2;
        nw->cx = cur->cx;
        nw->cy = cur->cy;
        nw->screen_x = cur->screen_x;
        nw->row_off = cur->row_off;
        nw->col_off = cur->col_off;

        window_load_active(new_idx);
        set_status("Split vertical (Win %d)", nw->id);
        return true;
    }
}

bool window_close_current(void) {
    if (E.num_windows <= 1) {
        set_status("Cannot close last window");
        return false;
    }
    window_save_active();
    int closed_idx = E.cur_win_idx;
    EditorWindow closed = E.windows[closed_idx];

    /* Find target window to absorb closed space */
    int target_idx = (closed_idx == 0) ? 1 : closed_idx - 1;
    EditorWindow *target = &E.windows[target_idx];
    if (closed.left == target->left && closed.cols == target->cols) {
        target->rows += closed.rows + 1;
        if (closed.top < target->top) target->top = closed.top;
    } else if (closed.top == target->top && closed.rows == target->rows) {
        target->cols += closed.cols + 1;
        if (closed.left < target->left) target->left = closed.left;
    } else {
        target->rows = E.screen_rows;
        target->cols = E.screen_cols;
        target->top = 0;
        target->left = 0;
    }

    for (int i = closed_idx; i < E.num_windows - 1; i++) {
        E.windows[i] = E.windows[i + 1];
    }
    E.num_windows--;

    int next_idx = (closed_idx >= E.num_windows) ? E.num_windows - 1 : closed_idx;
    window_load_active(next_idx);
    set_status("Window closed");
    return true;
}

void window_next(void) {
    if (E.num_windows <= 1) return;
    window_save_active();
    int next = (E.cur_win_idx + 1) % E.num_windows;
    window_load_active(next);
}

void window_prev(void) {
    if (E.num_windows <= 1) return;
    window_save_active();
    int prev = (E.cur_win_idx - 1 + E.num_windows) % E.num_windows;
    window_load_active(prev);
}

void window_move_dir(char dir) {
    if (E.num_windows <= 1) return;
    window_save_active();
    EditorWindow *cur = &E.windows[E.cur_win_idx];
    int best_idx = -1;
    int best_dist = 999999;

    for (int i = 0; i < E.num_windows; i++) {
        if (i == E.cur_win_idx) continue;
        EditorWindow *w = &E.windows[i];
        if (dir == 'h' && w->left < cur->left) {
            int dist = cur->left - (w->left + w->cols);
            if (dist >= 0 && dist < best_dist) { best_dist = dist; best_idx = i; }
        } else if (dir == 'l' && w->left > cur->left) {
            int dist = w->left - (cur->left + cur->cols);
            if (dist >= 0 && dist < best_dist) { best_dist = dist; best_idx = i; }
        } else if (dir == 'k' && w->top < cur->top) {
            int dist = cur->top - (w->top + w->rows);
            if (dist >= 0 && dist < best_dist) { best_dist = dist; best_idx = i; }
        } else if (dir == 'j' && w->top > cur->top) {
            int dist = w->top - (cur->top + cur->rows);
            if (dist >= 0 && dist < best_dist) { best_dist = dist; best_idx = i; }
        }
    }
    if (best_idx != -1) {
        window_load_active(best_idx);
    } else {
        window_next();
    }
}

void window_balance(void) {
    if (E.num_windows <= 1) return;
    int top_offset = (E.num_tabs > 1) ? 1 : 0;
    int avail_rows = E.screen_rows - top_offset - (E.num_windows - 1);
    if (avail_rows < E.num_windows) avail_rows = E.num_windows;
    int r = avail_rows / E.num_windows;
    int rem = avail_rows % E.num_windows;
    int top = top_offset;
    for (int i = 0; i < E.num_windows; i++) {
        E.windows[i].top = top;
        E.windows[i].left = 0;
        E.windows[i].cols = E.screen_cols;
        E.windows[i].rows = r + (i < rem ? 1 : 0);
        top += E.windows[i].rows + 1;
    }
}

int window_find_at(int screen_x, int screen_y) {
    int row = screen_y - 1;
    int col = screen_x - 1;
    for (int i = 0; i < E.num_windows; i++) {
        EditorWindow *w = &E.windows[i];
        if (row >= w->top && row < w->top + w->rows &&
            col >= w->left && col < w->left + w->cols) {
            return i;
        }
    }
    return -1;
}

