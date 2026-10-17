#include "vi.h"
#include "fold.h"
#include "screen.h"

void fold_init(void) {
    E.num_folds = 0;
    memset(E.folds, 0, sizeof(E.folds));
}

static int get_line_indent(const Line *l, int tabstop) {
    if (!l || l->len == 0) return -1;
    int indent = 0;
    int i = 0;
    while (i < l->len) {
        if (l->chars[i] == ' ') indent++;
        else if (l->chars[i] == '\t') indent += tabstop - (indent % tabstop);
        else break;
        i++;
    }
    if (i == l->len) return -1;
    return indent;
}

void fold_update(void) {
    int sw = (E.opt.shiftwidth > 0) ? E.opt.shiftwidth : 4;
    int ts = (E.opt.tabstop > 0) ? E.opt.tabstop : 4;

    /* Remember collapsed states by start line */
    bool collapsed_lines[1024] = {false};
    for (int i = 0; i < E.num_folds; i++) {
        if (E.folds[i].collapsed && E.folds[i].start_y < 1024) {
            collapsed_lines[E.folds[i].start_y] = true;
        }
    }

    E.num_folds = 0;

    for (int r = 0; r < E.num_lines - 1 && E.num_folds < MAX_FOLDS; r++) {
        int cur_indent = get_line_indent(&E.lines[r], ts);
        if (cur_indent < 0) continue;

        int next_r = r + 1;
        while (next_r < E.num_lines && get_line_indent(&E.lines[next_r], ts) < 0) {
            next_r++;
        }
        if (next_r >= E.num_lines) break;

        int next_indent = get_line_indent(&E.lines[next_r], ts);
        if (next_indent > cur_indent) {
            /* Found start of a fold at line r */
            int fold_end = next_r;
            for (int k = next_r + 1; k < E.num_lines; k++) {
                int ind = get_line_indent(&E.lines[k], ts);
                if (ind < 0) continue; /* Blank line continues fold */
                if (ind > cur_indent) {
                    fold_end = k;
                } else {
                    break;
                }
            }

            if (fold_end > r) {
                Fold *f = &E.folds[E.num_folds++];
                f->start_y = r;
                f->end_y = fold_end;
                f->indent = cur_indent / sw;
                f->collapsed = (r < 1024) ? collapsed_lines[r] : false;
            }
        }
    }
}

Fold *fold_find_at(int cy) {
    Fold *best = NULL;
    for (int i = 0; i < E.num_folds; i++) {
        Fold *f = &E.folds[i];
        if (cy >= f->start_y && cy <= f->end_y) {
            if (!best || (f->end_y - f->start_y) < (best->end_y - best->start_y)) {
                best = f;
            }
        }
    }
    return best;
}

Fold *fold_find_start(int cy) {
    for (int i = 0; i < E.num_folds; i++) {
        if (E.folds[i].start_y == cy && E.folds[i].collapsed) {
            return &E.folds[i];
        }
    }
    return NULL;
}

bool fold_is_line_hidden(int cy) {
    for (int i = 0; i < E.num_folds; i++) {
        Fold *f = &E.folds[i];
        if (f->collapsed && cy > f->start_y && cy <= f->end_y) {
            return true;
        }
    }
    return false;
}

bool fold_close_at(int cy) {
    fold_update();
    Fold *f = fold_find_at(cy);
    if (!f) {
        set_status("No fold found");
        return false;
    }
    f->collapsed = true;
    set_status("+-- %d lines folded", f->end_y - f->start_y + 1);
    return true;
}

bool fold_open_at(int cy) {
    fold_update();
    Fold *f = fold_find_at(cy);
    if (!f) {
        set_status("No fold found");
        return false;
    }
    f->collapsed = false;
    set_status("Fold opened");
    return true;
}

bool fold_toggle_at(int cy) {
    fold_update();
    Fold *f = fold_find_at(cy);
    if (!f) {
        set_status("No fold found");
        return false;
    }
    f->collapsed = !f->collapsed;
    if (f->collapsed) {
        set_status("+-- %d lines folded", f->end_y - f->start_y + 1);
    } else {
        set_status("Fold opened");
    }
    return true;
}

void fold_open_all(void) {
    for (int i = 0; i < E.num_folds; i++) {
        E.folds[i].collapsed = false;
    }
    set_status("All folds opened");
}

void fold_close_all(void) {
    fold_update();
    for (int i = 0; i < E.num_folds; i++) {
        E.folds[i].collapsed = true;
    }
    set_status("All folds closed");
}
