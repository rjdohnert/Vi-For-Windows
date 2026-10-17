// Copyright (C) 2020-2026, PC/OpenSystems LLC
// Licensed under the terms of the BSD-3 Clause license

#include "vi.h"
#include "term.h"
#include "buffer.h"
#include "undo.h"
#include "reg.h"
#include "search.h"
#include "motion.h"
#include "ops.h"
#include "ex.h"
#include "screen.h"
#include "window.h"
#include "tags.h"
#include "explorer.h"
#include "syntax.h"
#include "diff.h"
#include "quickfix.h"
#include "session.h"
#include "tab.h"
#include "fold.h"

Editor E;

static void history_add(History *h, const char *s) {
    if (!s || *s == '\0') return;
    if (h->count > 0 && strcmp(h->items[h->count - 1], s) == 0) return;
    if (h->count < MAX_HIST) {
        strncpy(h->items[h->count], s, sizeof(h->items[0]) - 1);
        h->items[h->count][sizeof(h->items[0]) - 1] = '\0';
        h->count++;
    } else {
        for (int i = 0; i < MAX_HIST - 1; i++) {
            strcpy(h->items[i], h->items[i + 1]);
        }
        strncpy(h->items[MAX_HIST - 1], s, sizeof(h->items[0]) - 1);
        h->items[MAX_HIST - 1][sizeof(h->items[0]) - 1] = '\0';
    }
}

static char s_macro_record_buf[4096];
static int s_macro_record_len = 0;

static void macro_record_char(int k) {
    if (!E.recording_macro) return;
    if (s_macro_record_len >= (int)sizeof(s_macro_record_buf) - 2) return;
    if (k >= 0 && k < 256) {
        s_macro_record_buf[s_macro_record_len++] = (char)k;
    } else if (k == KEY_ARROW_UP) {
        s_macro_record_buf[s_macro_record_len++] = 'k';
    } else if (k == KEY_ARROW_DOWN) {
        s_macro_record_buf[s_macro_record_len++] = 'j';
    } else if (k == KEY_ARROW_LEFT) {
        s_macro_record_buf[s_macro_record_len++] = 'h';
    } else if (k == KEY_ARROW_RIGHT) {
        s_macro_record_buf[s_macro_record_len++] = 'l';
    } else if (k == KEY_DEL) {
        s_macro_record_buf[s_macro_record_len++] = 'x';
    }
    s_macro_record_buf[s_macro_record_len] = '\0';
}

static int utf8_char_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static bool utf8_is_continuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

static int editor_read_key(void) {
    int c = term_read_key();
    macro_record_char(c);
    return c;
}

static int s_search_orig_cy = 0;
static int s_search_orig_cx = 0;
static char s_search_saved_last[256] = {0};

static void incsearch_preview(void) {
    if (!E.opt.incsearch || E.cmd_len == 0 || E.num_lines == 0) return;
    int cur = s_search_orig_cy;
    int start_col = s_search_orig_cx;
    for (int step = 0; step < E.num_lines; step++) {
        int delta = E.search_forward ? step : -step;
        int row = ((cur + delta) % E.num_lines + E.num_lines) % E.num_lines;
        char *line = E.lines[row].chars;
        const char *pos = NULL;
        if (step == 0) {
            if (E.search_forward && start_col < E.lines[row].len) {
                pos = regex_search_line(E.cmd_buf, &line[start_col], E.opt.ignorecase, NULL);
            } else if (!E.search_forward && start_col >= 0) {
                for (int i = start_col; i >= 0; i--) {
                    if (regex_search_line(E.cmd_buf, &line[i], E.opt.ignorecase, NULL) == &line[i]) {
                        pos = &line[i];
                        break;
                    }
                }
            }
        } else {
            pos = regex_search_line(E.cmd_buf, line, E.opt.ignorecase, NULL);
            if (!E.search_forward && pos) {
                const char *last = pos;
                const char *next = pos + 1;
                while (*next && (next = regex_search_line(E.cmd_buf, next, E.opt.ignorecase, NULL))) {
                    last = next;
                    next++;
                }
                pos = last;
            }
        }
        if (pos) {
            E.cy = row;
            E.cx = (int)(pos - line);
            return;
        }
    }
    E.cy = s_search_orig_cy;
    E.cx = s_search_orig_cx;
}

static bool get_word_under_cursor(char *buf, size_t max_len) {
    if (E.cy < 0 || E.cy >= E.num_lines || E.lines[E.cy].len == 0) return false;
    Line *row = &E.lines[E.cy];
    int start = E.cx;
    if (start >= row->len) start = row->len - 1;
    if (!isalnum((unsigned char)row->chars[start]) && row->chars[start] != '_') {
        while (start < row->len && !isalnum((unsigned char)row->chars[start]) && row->chars[start] != '_') {
            start++;
        }
        if (start >= row->len) return false;
    }
    while (start > 0 && (isalnum((unsigned char)row->chars[start - 1]) || row->chars[start - 1] == '_')) {
        start--;
    }
    int end = start;
    while (end < row->len && (isalnum((unsigned char)row->chars[end]) || row->chars[end] == '_')) {
        end++;
    }
    if (end > start) {
        int len = min(end - start, (int)max_len - 1);
        strncpy(buf, &row->chars[start], len);
        buf[len] = '\0';
        return true;
    }
    return false;
}

void process_keypress(void) {
    int c = editor_read_key();
    if (c == 0) return;

    if (E.recording_macro && E.mode == MODE_NORMAL && c == 'q') {
        if (s_macro_record_len > 0) s_macro_record_len--;
        s_macro_record_buf[s_macro_record_len] = '\0';
        E.recording_macro = false;
        reg_set(E.macro_reg, s_macro_record_buf, s_macro_record_len, VI_REG_CHAR);
        set_status("");
        return;
    }

    /* HELP Mode */
    if (E.mode == MODE_HELP) {
        if (c == 'q' || c == 27) {
            E.mode = MODE_NORMAL;
        } else if (c == 'j' || c == KEY_ARROW_DOWN) {
            E.help_scroll++;
        } else if (c == 'k' || c == KEY_ARROW_UP) {
            if (E.help_scroll > 0) E.help_scroll--;
        } else if (c == KEY_MOUSE_WHEEL_DOWN) {
            E.help_scroll += 3;
        } else if (c == KEY_MOUSE_WHEEL_UP) {
            if (E.help_scroll > 3) E.help_scroll -= 3;
            else E.help_scroll = 0;
        }
        return;
    }

    /* EX Line-Editor Mode */
    if (E.mode == MODE_EX) {
        run_ex_mode();
        return;
    }

    /* Ignore mouse release across all modes */
    if (c == KEY_MOUSE_RELEASE) {
        return;
    }

    /* Disregard mouse events if mouse option is disabled */
    if (!E.opt.mouse && (c == KEY_MOUSE_PRESS || c == KEY_MOUSE_WHEEL_UP || c == KEY_MOUSE_WHEEL_DOWN)) {
        return;
    }

    /* Mouse Wheel Scrolling */
    if (c == KEY_MOUSE_WHEEL_UP || c == KEY_MOUSE_WHEEL_DOWN) {
        if (E.mode == MODE_COMMAND || E.mode == MODE_SEARCH) return;
        int target_win = window_find_at(E.mouse_x, E.mouse_y);
        if (target_win != -1 && target_win != E.cur_win_idx) {
            window_save_active();
            window_load_active(target_win);
        }
        int win_rows = (E.cur_win_idx >= 0 && E.cur_win_idx < E.num_windows) ? E.windows[E.cur_win_idx].rows : E.screen_rows;
        if (c == KEY_MOUSE_WHEEL_UP) {
            for (int step = 0; step < 3; step++) {
                if (E.row_off > 0) E.row_off--;
                if (E.cy >= E.row_off + win_rows) E.cy = max(0, E.row_off + win_rows - 1);
            }
        } else {
            for (int step = 0; step < 3; step++) {
                if (E.row_off < E.num_lines - 1) E.row_off++;
                if (E.cy < E.row_off) E.cy = E.row_off;
            }
        }
        int max_x = (E.mode == MODE_INSERT) ? (E.lines ? E.lines[E.cy].len : 0) :
                    (E.lines && E.lines[E.cy].len > 0 ? E.lines[E.cy].len - 1 : 0);
        if (E.cx > max_x) E.cx = max_x;
        return;
    }

    /* Mouse Click (Press) */
    if (c == KEY_MOUSE_PRESS) {
        if (E.mode == MODE_COMMAND || E.mode == MODE_SEARCH) {
            if (E.mode == MODE_SEARCH && E.opt.incsearch) {
                E.cy = s_search_orig_cy;
                E.cx = s_search_orig_cx;
                strncpy(E.last_search, s_search_saved_last, sizeof(E.last_search) - 1);
            }
            E.mode = MODE_NORMAL;
            E.cmd_buf[0] = '\0';
            E.cmd_len = 0;
        }

        E.pending_op = 0;
        E.count1 = 0;
        E.count2 = 0;

        /* Check Tabline click (row 1 when multiple tabs are open) */
        if (E.num_tabs > 1 && E.mouse_y == 1) {
            tab_handle_click(E.mouse_x);
            return;
        }

        /* Check which window was clicked */
        int win_idx = window_find_at(E.mouse_x, E.mouse_y);
        if (win_idx == -1) {
            /* Check window status bars */
            int row = E.mouse_y - 1;
            int col = E.mouse_x - 1;
            for (int i = 0; i < E.num_windows; i++) {
                EditorWindow *w = &E.windows[i];
                if (row == w->top + w->rows && col >= w->left && col < w->left + w->cols) {
                    win_idx = i;
                    break;
                }
            }
        }

        if (win_idx != -1) {
            if (win_idx != E.cur_win_idx) {
                window_save_active();
                window_load_active(win_idx);
            }

            EditorWindow *w = &E.windows[E.cur_win_idx];
            int rel_row = (E.mouse_y - 1) - w->top;
            int rel_col = (E.mouse_x - 1) - w->left;

            if (rel_row >= 0 && rel_row < w->rows && E.num_lines > 0) {
                int prefix_cols = E.opt.number ? 7 : (w->diff_mode ? 2 : 0);
                int target_cy = w->row_off + rel_row;
                if (target_cy >= E.num_lines) target_cy = max(0, E.num_lines - 1);
                E.cy = target_cy;

                int col_in_text = rel_col - prefix_cols;
                if (col_in_text < 0) col_in_text = 0;
                int target_cx = w->col_off + col_in_text;
                int max_x = (E.mode == MODE_INSERT) ? E.lines[E.cy].len :
                            (E.lines[E.cy].len > 0 ? E.lines[E.cy].len - 1 : 0);
                if (target_cx > max_x) target_cx = max_x;
                if (target_cx < 0) target_cx = 0;
                E.cx = target_cx;
            }

            /* Right-click: paste from system clipboard */
            if ((E.mouse_b & 3) == 2) {
                char old_reg = E.active_reg;
                E.active_reg = '+';
                do_paste(false, 1);
                E.active_reg = old_reg;
            }
        }
        return;
    }

    /* VISUAL Modes (characterwise, linewise, blockwise) */
    if (E.mode == MODE_VISUAL || E.mode == MODE_VISUAL_LINE || E.mode == MODE_VISUAL_BLOCK) {
        if (c == 27) { /* ESC */
            E.mode = MODE_NORMAL;
            return;
        } else if (c == 'v') {
            E.mode = (E.mode == MODE_VISUAL) ? MODE_NORMAL : MODE_VISUAL;
            return;
        } else if (c == 'V') {
            E.mode = (E.mode == MODE_VISUAL_LINE) ? MODE_NORMAL : MODE_VISUAL_LINE;
            return;
        } else if (c == 22 || c == 17) { /* Ctrl-V or Ctrl-Q */
            E.mode = (E.mode == MODE_VISUAL_BLOCK) ? MODE_NORMAL : MODE_VISUAL_BLOCK;
            return;
        } else if (c == 'i' || c == 'a') {
            /* Text object in visual mode */
            char target = (char)editor_read_key();
            int ty1, tx1, ty2, tx2;
            if (evaluate_text_object((char)c, target, &ty1, &tx1, &ty2, &tx2)) {
                E.visual_anchor_y = ty1;
                E.visual_anchor_x = tx1;
                E.cy = ty2;
                E.cx = tx2;
            }
            return;
        } else if (E.mode == MODE_VISUAL_BLOCK && (c == 'I' || c == 'A')) {
            int y1 = min(E.visual_anchor_y, E.cy);
            int y2 = max(E.visual_anchor_y, E.cy);
            int x1 = min(E.visual_anchor_x, E.cx);
            int x2 = max(E.visual_anchor_x, E.cx);
            int ins_col = (c == 'I') ? x1 : x2 + 1;
            E.cy = y1;
            E.cx = ins_col;
            E.mode = MODE_INSERT;
            E.curr_action.type = ACT_INSERT;
            E.curr_action.count = y2 - y1 + 1;
            E.curr_action.motion = 'B';
            E.curr_action.arg_char = (char)ins_col;
            return;
        } else if (c == 'd' || c == 'x' || c == 'y' || c == 'c' || c == '>' || c == '<' || c == '~') {
            char op = (c == 'x') ? 'd' : (char)c;
            if (E.mode == MODE_VISUAL_BLOCK) {
                apply_block_operator(op);
                return;
            }
            bool linewise = (E.mode == MODE_VISUAL_LINE);
            int y1 = E.visual_anchor_y, x1 = E.visual_anchor_x;
            int y2 = E.cy, x2 = E.cx;
            if (!linewise) {
                if (y1 < y2 || (y1 == y2 && x1 <= x2)) {
                    x2++;
                } else {
                    x1++;
                }
            }
            apply_operator_on_range(op, y1, x1, y2, x2, linewise);
            if (op == 'c') E.mode = MODE_INSERT;
            else E.mode = MODE_NORMAL;
            return;
        } else if (c == 'u' || c == 'U') {
            int y1 = E.visual_anchor_y, x1 = E.visual_anchor_x;
            int y2 = E.cy, x2 = E.cx;
            if (y1 > y2 || (y1 == y2 && x1 > x2)) {
                int ty = y1; y1 = y2; y2 = ty;
                int tx = x1; x1 = x2; x2 = tx;
            }
            save_undo();
            for (int r = y1; r <= y2 && r < E.num_lines; r++) {
                int start_x, end_x;
                if (E.mode == MODE_VISUAL_LINE) {
                    start_x = 0;
                    end_x = E.lines[r].len - 1;
                } else if (E.mode == MODE_VISUAL_BLOCK) {
                    start_x = min(x1, x2);
                    end_x = max(x1, x2);
                } else {
                    start_x = (r == y1) ? x1 : 0;
                    end_x = (r == y2) ? x2 : E.lines[r].len - 1;
                }
                for (int j = start_x; j <= end_x && j < E.lines[r].len; j++) {
                    if (c == 'u') E.lines[r].chars[j] = (char)tolower((unsigned char)E.lines[r].chars[j]);
                    else E.lines[r].chars[j] = (char)toupper((unsigned char)E.lines[r].chars[j]);
                }
            }
            E.dirty = true;
            E.mode = MODE_NORMAL;
            return;
        }

        /* Navigate/expand visual selection with motions */
        if (c == 'f' || c == 'F' || c == 't' || c == 'T') {
            char target = (char)editor_read_key();
            E.inline_cmd = (char)c;
            E.inline_char = target;
        }
        MotionResult m = evaluate_motion(c, 1);
        if (m.valid) {
            E.cy = m.y;
            E.cx = m.x;
            if (E.cy < E.num_lines && E.cx >= E.lines[E.cy].len && E.lines[E.cy].len > 0) {
                E.cx = E.lines[E.cy].len - 1;
            }
        }
        return;
    }

    /* COMMAND / SEARCH Prompt */
    if (E.mode == MODE_COMMAND || E.mode == MODE_SEARCH) {
        if (c == 27) {
            if (E.mode == MODE_SEARCH && E.opt.incsearch) {
                E.cy = s_search_orig_cy;
                E.cx = s_search_orig_cx;
                strncpy(E.last_search, s_search_saved_last, sizeof(E.last_search) - 1);
            }
            E.mode = MODE_NORMAL;
            E.cmd_buf[0] = '\0';
            E.cmd_len = 0;
        } else if (c == KEY_ARROW_UP) {
            History *h = (E.mode == MODE_COMMAND) ? &E.cmd_hist : &E.search_hist;
            if (E.hist_idx > 0 && h->count > 0) {
                E.hist_idx--;
                strncpy(E.cmd_buf, h->items[E.hist_idx], sizeof(E.cmd_buf) - 1);
                E.cmd_buf[sizeof(E.cmd_buf) - 1] = '\0';
                E.cmd_len = (int)strlen(E.cmd_buf);
            }
        } else if (c == KEY_ARROW_DOWN) {
            History *h = (E.mode == MODE_COMMAND) ? &E.cmd_hist : &E.search_hist;
            if (E.hist_idx < h->count - 1) {
                E.hist_idx++;
                strncpy(E.cmd_buf, h->items[E.hist_idx], sizeof(E.cmd_buf) - 1);
                E.cmd_buf[sizeof(E.cmd_buf) - 1] = '\0';
                E.cmd_len = (int)strlen(E.cmd_buf);
            } else {
                E.hist_idx = h->count;
                E.cmd_buf[0] = '\0';
                E.cmd_len = 0;
            }
        } else if (c == '\r' || c == '\n') {
            if (E.cmd_len > 0) {
                if (E.mode == MODE_COMMAND) history_add(&E.cmd_hist, E.cmd_buf);
                else history_add(&E.search_hist, E.cmd_buf);
            }
            if (E.mode == MODE_COMMAND) {
                execute_ex_command(E.cmd_buf);
            } else {
                strncpy(E.last_search, E.cmd_buf, sizeof(E.last_search) - 1);
                E.opt.nohl_active = false;
                E.cy = s_search_orig_cy;
                E.cx = s_search_orig_cx;
                editor_search(E.search_forward);
            }
            if (E.mode != MODE_HELP && E.mode != MODE_EX) E.mode = MODE_NORMAL;
            E.cmd_buf[0] = '\0';
            E.cmd_len = 0;
        } else if (c == '\b' || c == 127) {
            if (E.cmd_len > 0) {
                E.cmd_buf[--E.cmd_len] = '\0';
                if (E.mode == MODE_SEARCH && E.opt.incsearch) {
                    if (E.cmd_len > 0) {
                        strncpy(E.last_search, E.cmd_buf, sizeof(E.last_search) - 1);
                        incsearch_preview();
                    } else {
                        E.cy = s_search_orig_cy;
                        E.cx = s_search_orig_cx;
                        strncpy(E.last_search, s_search_saved_last, sizeof(E.last_search) - 1);
                    }
                }
            } else {
                if (E.mode == MODE_SEARCH && E.opt.incsearch) {
                    E.cy = s_search_orig_cy;
                    E.cx = s_search_orig_cx;
                    strncpy(E.last_search, s_search_saved_last, sizeof(E.last_search) - 1);
                }
                E.mode = MODE_NORMAL;
            }
        } else if (isprint(c) && E.cmd_len < (int)sizeof(E.cmd_buf) - 1) {
            E.cmd_buf[E.cmd_len++] = (char)c;
            E.cmd_buf[E.cmd_len] = '\0';
            if (E.mode == MODE_SEARCH && E.opt.incsearch) {
                strncpy(E.last_search, E.cmd_buf, sizeof(E.last_search) - 1);
                incsearch_preview();
            }
        }
        return;
    }

    /* INSERT and REPLACE Modes */
    if (E.mode == MODE_INSERT || E.mode == MODE_REPLACE) {
        if (c == 14) { /* Ctrl-N */
            completion_trigger(1);
            return;
        } else if (c == 16) { /* Ctrl-P */
            completion_trigger(-1);
            return;
        }
        completion_reset();

        if (c == 27) {
            if (E.cx > 0) E.cx--;
            E.mode = MODE_NORMAL;

            if (E.curr_action.motion == 'B' && E.curr_action.count > 1 && E.curr_action.inserted_len > 0) {
                int ins_col = (int)(unsigned char)E.curr_action.arg_char;
                for (int r = 1; r < E.curr_action.count; r++) {
                    int row = E.cy + r;
                    if (row < E.num_lines) {
                        Line *line = &E.lines[row];
                        while (line->len < ins_col) line_insert_char(line, line->len, ' ');
                        line_insert_str(line, ins_col, E.curr_action.inserted, E.curr_action.inserted_len);
                    }
                }
                E.curr_action.motion = 0;
            } else if (E.curr_action.count > 1 && E.curr_action.inserted_len > 0) {
                for (int n = 1; n < E.curr_action.count; n++) {
                    const char *p = E.curr_action.inserted;
                    while (*p) {
                        if (*p == '\r' || *p == '\n') {
                            Line *row = &E.lines[E.cy];
                            insert_row(E.cy + 1, &row->chars[E.cx + 1], row->len - (E.cx + 1));
                            row = &E.lines[E.cy];
                            row->len = E.cx + 1;
                            row->chars[row->len] = '\0';
                            E.cy++;
                            E.cx = 0;
                        } else {
                            line_insert_char(&E.lines[E.cy], ++E.cx, *p);
                        }
                        p++;
                    }
                }
            }
            finalize_repeat_action();
            backup_u_line();
            swap_write();
        } else if (c == KEY_ARROW_LEFT) {
            if (E.cx > 0) E.cx--;
        } else if (c == KEY_ARROW_RIGHT) {
            if (E.cx < E.lines[E.cy].len) E.cx++;
        } else if (c == KEY_ARROW_UP) {
            if (E.cy > 0) {
                E.cy--;
                if (E.cx > E.lines[E.cy].len) E.cx = E.lines[E.cy].len;
            }
        } else if (c == KEY_ARROW_DOWN) {
            if (E.cy < E.num_lines - 1) {
                E.cy++;
                if (E.cx > E.lines[E.cy].len) E.cx = E.lines[E.cy].len;
            }
        } else if (c == KEY_DEL) {
            if (E.cx < E.lines[E.cy].len) {
                line_delete_char(&E.lines[E.cy], E.cx);
            }
        } else if (c == '\r' || c == '\n') {
            record_insert_char('\n');
            Line *row = &E.lines[E.cy];

            /* Auto-indent calculation */
            char indent_buf[128] = {0};
            int indent_len = 0;
            if (E.opt.autoindent) {
                while (indent_len < row->len && indent_len < (int)sizeof(indent_buf) - 1 &&
                       (row->chars[indent_len] == ' ' || row->chars[indent_len] == '\t')) {
                    indent_buf[indent_len] = row->chars[indent_len];
                    indent_len++;
                }
                indent_buf[indent_len] = '\0';
            }

            insert_row(E.cy + 1, &row->chars[E.cx], row->len - E.cx);
            row = &E.lines[E.cy];
            row->len = E.cx;
            row->chars[row->len] = '\0';
            E.cy++;
            E.cx = 0;

            if (indent_len > 0) {
                line_insert_str(&E.lines[E.cy], 0, indent_buf, indent_len);
                E.cx = indent_len;
                for (int i = 0; i < indent_len; i++) record_insert_char(indent_buf[i]);
            }
        } else if (c == '\b' || c == 127) {
            if (E.cx > 0) {
                int del_bytes = 1;
                while (E.cx - del_bytes > 0 && utf8_is_continuation((unsigned char)E.lines[E.cy].chars[E.cx - del_bytes])) del_bytes++;
                for (int k = 0; k < del_bytes; k++) {
                    line_delete_char(&E.lines[E.cy], E.cx - del_bytes);
                }
                E.cx -= del_bytes;
            } else if (E.cy > 0) {
                int prev_len = E.lines[E.cy - 1].len;
                Line *cur = &E.lines[E.cy];
                for (int i = 0; i < cur->len; i++) {
                    line_insert_char(&E.lines[E.cy - 1], prev_len + i, cur->chars[i]);
                }
                delete_row(E.cy);
                E.cy--;
                E.cx = prev_len;
            }
        } else if (c == '\t') {
            for (int i = 0; i < E.opt.tabstop; i++) {
                record_insert_char(' ');
                line_insert_char(&E.lines[E.cy], E.cx++, ' ');
            }
        } else if (isprint(c)) {
            record_insert_char((char)c);
            if (E.mode == MODE_REPLACE && E.cx < E.lines[E.cy].len) {
                E.lines[E.cy].chars[E.cx] = (char)c;
                E.dirty = true;
                E.cx++;
            } else {
                line_insert_char(&E.lines[E.cy], E.cx, (char)c);
                E.cx++;
            }
        }
        return;
    }

    /* --- NORMAL MODE --- */

    if (c == KEY_F1) {
        E.mode = MODE_HELP;
        E.help_scroll = 0;
        return;
    }

    /* Handle arrow and navigation keys in normal mode */
    if (c == KEY_ARROW_UP) c = 'k';
    else if (c == KEY_ARROW_DOWN) c = 'j';
    else if (c == KEY_ARROW_LEFT) c = 'h';
    else if (c == KEY_ARROW_RIGHT) c = 'l';
    else if (c == KEY_HOME) c = '^';
    else if (c == KEY_END) c = '$';
    else if (c == KEY_PAGE_UP) c = 2;   /* Ctrl-B */
    else if (c == KEY_PAGE_DOWN) c = 6; /* Ctrl-F */
    else if (c == KEY_DEL) c = 'x';

    if (c == '\r' || c == '\n') {
        if (explorer_handle_enter()) {
            E.count1 = 0;
            E.count2 = 0;
            return;
        }
        if (quickfix_handle_enter()) {
            E.count1 = 0;
            E.count2 = 0;
            return;
        }
    } else if (c == '-' && !E.pending_op && E.filename && strncmp(E.filename, "[Explorer: ", 11) == 0) {
        E.cy = 4;
        if (explorer_handle_enter()) {
            E.count1 = 0;
            E.count2 = 0;
            return;
        }
    }

    if (isdigit((unsigned char)c)) {
        if (c == '0' && ((!E.pending_op && E.count1 == 0) || (E.pending_op && E.count2 == 0))) {
            /* '0' by itself is beginning of line motion */
        } else {
            if (E.pending_op) {
                E.count2 = E.count2 * 10 + (c - '0');
            } else {
                E.count1 = E.count1 * 10 + (c - '0');
            }
            return;
        }
    }

    if (c == '"') {
        int r = editor_read_key();
        if (isprint(r) || r == '+' || r == '*') E.active_reg = (char)r;
        return;
    }

    int total_count = (E.count1 > 0 ? E.count1 : 1) * (E.count2 > 0 ? E.count2 : 1);

    if (E.pending_op) {
        char op = E.pending_op;
        E.pending_op = 0;

        if (c == op) {
            int start_y = E.cy;
            int end_y = min(E.num_lines - 1, E.cy + total_count - 1);
            apply_operator_on_range(op, start_y, 0, end_y, 0, true);

            E.curr_action.type = (op == 'c') ? ACT_CHANGE_MOTION : ACT_DIRECT;
            E.curr_action.op = op;
            E.curr_action.count = total_count;
            if (op != 'c') finalize_repeat_action();
        } else if (c == 'i' || c == 'a') {
            char target = (char)editor_read_key();
            int y1, x1, y2, x2;
            if (evaluate_text_object((char)c, target, &y1, &x1, &y2, &x2)) {
                apply_operator_on_range(op, y1, x1, y2, x2 + 1, target == 'p');

                E.curr_action.type = (op == 'c') ? ACT_CHANGE_MOTION : ACT_DELETE_MOTION;
                E.curr_action.op = op;
                E.curr_action.count = total_count;
                E.curr_action.motion = (char)c;
                E.curr_action.arg_char = target;
                if (op != 'c') finalize_repeat_action();
            }
        } else {
            char motion_arg = 0;
            if (c == 'f' || c == 'F' || c == 't' || c == 'T') {
                motion_arg = (char)editor_read_key();
                E.inline_cmd = (char)c;
                E.inline_char = motion_arg;
            }

            MotionResult m = evaluate_motion(c, total_count);
            if (m.valid) {
                int y1 = E.cy, x1 = E.cx;
                int y2 = m.y, x2 = m.x;
                if (!m.linewise && m.inclusive) {
                    if (y1 < y2 || (y1 == y2 && x1 <= x2)) {
                        x2++;
                    } else {
                        x1++;
                    }
                }
                apply_operator_on_range(op, y1, x1, y2, x2, m.linewise);

                E.curr_action.type = (op == 'c') ? ACT_CHANGE_MOTION : ACT_DELETE_MOTION;
                E.curr_action.op = op;
                E.curr_action.count = E.count1;
                E.curr_action.motion = (char)c;
                E.curr_action.motion_count = E.count2 > 0 ? E.count2 : total_count;
                E.curr_action.arg_char = motion_arg;
                if (op != 'c') finalize_repeat_action();
            }
        }

        E.count1 = 0;
        E.count2 = 0;
        backup_u_line();
        return;
    }

    switch (c) {
        case 'd':
        case 'c':
        case 'y':
        case '>':
        case '<':
            E.pending_op = (char)c;
            return;

        case '.':
            repeat_last_action(E.count1);
            E.count1 = 0;
            backup_u_line();
            return;

        case 7: { /* Ctrl-G */
            int pct = (E.num_lines > 0) ? ((E.cy + 1) * 100 / E.num_lines) : 100;
            set_status("\"%s\"%s%s%s line %d of %d --%d%%--",
                       E.filename ? E.filename : "No File",
                       E.readonly ? " [readonly]" : "",
                       E.dirty ? " [Modified]" : "",
                       E.is_crlf ? " [dos]" : "",
                       E.cy + 1, E.num_lines, pct);
            break;
        }

        case 6: { /* Ctrl-F */
            E.cy = min(E.num_lines - 1, E.cy + max(1, E.screen_rows - 2));
            E.row_off = min(max(0, E.num_lines - 1), E.row_off + max(1, E.screen_rows - 2));
            break;
        }
        case 2: { /* Ctrl-B */
            E.cy = max(0, E.cy - max(1, E.screen_rows - 2));
            E.row_off = max(0, E.row_off - max(1, E.screen_rows - 2));
            break;
        }
        case 4: { /* Ctrl-D */
            int delta = max(1, E.screen_rows / 2);
            E.cy = min(E.num_lines - 1, E.cy + delta);
            E.row_off = min(max(0, E.num_lines - 1), E.row_off + delta);
            break;
        }
        case 21: { /* Ctrl-U */
            int delta = max(1, E.screen_rows / 2);
            E.cy = max(0, E.cy - delta);
            E.row_off = max(0, E.row_off - delta);
            break;
        }
        case 5: { /* Ctrl-E */
            if (E.row_off < E.num_lines - 1) E.row_off++;
            if (E.cy < E.row_off) E.cy = E.row_off;
            break;
        }
        case 25: { /* Ctrl-Y */
            if (E.row_off > 0) E.row_off--;
            if (E.cy >= E.row_off + E.screen_rows) E.cy = E.row_off + E.screen_rows - 1;
            break;
        }
        case 12: /* Ctrl-L */
            printf("\x1b[2J\x1b[H");
            break;

        case 'z': {
            int sub = editor_read_key();
            if (sub == '\r' || sub == '\n') E.row_off = E.cy;
            else if (sub == '.') E.row_off = max(0, E.cy - E.screen_rows / 2);
            else if (sub == '-') E.row_off = max(0, E.cy - E.screen_rows + 1);
            else if (sub == 'c') fold_close_at(E.cy);
            else if (sub == 'o') fold_open_at(E.cy);
            else if (sub == 'a') fold_toggle_at(E.cy);
            else if (sub == 'R') fold_open_all();
            else if (sub == 'M') fold_close_all();
            break;
        }

        case 'm': {
            int m = editor_read_key();
            if (m >= 'a' && m <= 'z') {
                E.marks[m - 'a'].y = E.cy;
                E.marks[m - 'a'].x = E.cx;
                E.marks[m - 'a'].set = true;
                set_status("Mark '%c' set", m);
            }
            break;
        }
        case '\'': {
            int m = editor_read_key();
            if (m == '\'') {
                if (E.mark_prev.set) {
                    E.cy = E.mark_prev.y;
                    E.cx = E.mark_prev.x;
                }
            } else if (m >= 'a' && m <= 'z' && E.marks[m - 'a'].set) {
                E.cy = E.marks[m - 'a'].y;
                E.cx = 0;
                while (E.cx < E.lines[E.cy].len && isblank(E.lines[E.cy].chars[E.cx])) E.cx++;
            }
            break;
        }
        case '`': {
            int m = editor_read_key();
            if (m == '`') {
                if (E.mark_prev.set) {
                    E.cy = E.mark_prev.y;
                    E.cx = E.mark_prev.x;
                }
            } else if (m >= 'a' && m <= 'z' && E.marks[m - 'a'].set) {
                E.cy = E.marks[m - 'a'].y;
                E.cx = E.marks[m - 'a'].x;
            }
            break;
        }

        case 'f':
        case 'F':
        case 't':
        case 'T': {
            char target = (char)editor_read_key();
            E.inline_cmd = (char)c;
            E.inline_char = target;
            MotionResult m = motion_inline_find((char)c, target, total_count);
            if (m.valid) E.cx = m.x;
            break;
        }

        case 'i':
            save_undo();
            E.curr_action.type = ACT_INSERT;
            E.curr_action.op = 'i';
            E.curr_action.count = total_count;
            E.mode = MODE_INSERT;
            break;
        case 'I':
            save_undo();
            E.cx = 0;
            while (E.cx < E.lines[E.cy].len && isblank(E.lines[E.cy].chars[E.cx])) E.cx++;
            E.curr_action.type = ACT_INSERT;
            E.curr_action.op = 'I';
            E.curr_action.count = total_count;
            E.mode = MODE_INSERT;
            break;
        case 'a':
            save_undo();
            if (E.lines[E.cy].len > 0) E.cx++;
            E.curr_action.type = ACT_INSERT;
            E.curr_action.op = 'a';
            E.curr_action.count = total_count;
            E.mode = MODE_INSERT;
            break;
        case 'A':
            save_undo();
            E.cx = E.lines[E.cy].len;
            E.curr_action.type = ACT_INSERT;
            E.curr_action.op = 'A';
            E.curr_action.count = total_count;
            E.mode = MODE_INSERT;
            break;
        case 'o': {
            save_undo();
            char indent_buf[128] = {0};
            int indent_len = 0;
            if (E.opt.autoindent && E.cy < E.num_lines) {
                Line *cur = &E.lines[E.cy];
                while (indent_len < cur->len && indent_len < (int)sizeof(indent_buf) - 1 &&
                       (cur->chars[indent_len] == ' ' || cur->chars[indent_len] == '\t')) {
                    indent_buf[indent_len] = cur->chars[indent_len];
                    indent_len++;
                }
                indent_buf[indent_len] = '\0';
            }
            insert_row(E.cy + 1, indent_buf, indent_len);
            E.cy++;
            E.cx = indent_len;
            E.curr_action.type = ACT_INSERT;
            E.curr_action.op = 'o';
            E.curr_action.count = total_count;
            E.mode = MODE_INSERT;
            break;
        }
        case 'O': {
            save_undo();
            char indent_buf[128] = {0};
            int indent_len = 0;
            if (E.opt.autoindent && E.cy < E.num_lines) {
                Line *cur = &E.lines[E.cy];
                while (indent_len < cur->len && indent_len < (int)sizeof(indent_buf) - 1 &&
                       (cur->chars[indent_len] == ' ' || cur->chars[indent_len] == '\t')) {
                    indent_buf[indent_len] = cur->chars[indent_len];
                    indent_len++;
                }
                indent_buf[indent_len] = '\0';
            }
            insert_row(E.cy, indent_buf, indent_len);
            E.cx = indent_len;
            E.curr_action.type = ACT_INSERT;
            E.curr_action.op = 'O';
            E.curr_action.count = total_count;
            E.mode = MODE_INSERT;
            break;
        }
        case 'R':
            save_undo();
            E.curr_action.type = ACT_INSERT;
            E.curr_action.op = 'R';
            E.curr_action.count = 1;
            E.mode = MODE_REPLACE;
            break;

        case 'x':
            save_undo();
            for (int i = 0; i < total_count; i++) {
                if (E.lines[E.cy].len > 0 && E.cx < E.lines[E.cy].len) {
                    int clen = utf8_char_len((unsigned char)E.lines[E.cy].chars[E.cx]);
                    for (int k = 0; k < clen; k++) {
                        line_delete_char(&E.lines[E.cy], E.cx);
                    }
                }
            }
            if (E.cx >= E.lines[E.cy].len && E.cx > 0) E.cx = E.lines[E.cy].len - 1;
            while (E.cx > 0 && utf8_is_continuation((unsigned char)E.lines[E.cy].chars[E.cx])) E.cx--;
            E.curr_action.type = ACT_DIRECT;
            E.curr_action.op = 'x';
            E.curr_action.count = total_count;
            finalize_repeat_action();
            break;
        case 'X':
            save_undo();
            for (int i = 0; i < total_count; i++) {
                if (E.cx > 0) {
                    int del_bytes = 1;
                    while (E.cx - del_bytes > 0 && utf8_is_continuation((unsigned char)E.lines[E.cy].chars[E.cx - del_bytes])) del_bytes++;
                    for (int k = 0; k < del_bytes; k++) {
                        line_delete_char(&E.lines[E.cy], E.cx - del_bytes);
                    }
                    E.cx -= del_bytes;
                }
            }
            E.curr_action.type = ACT_DIRECT;
            E.curr_action.op = 'X';
            E.curr_action.count = total_count;
            finalize_repeat_action();
            break;
        case 's':
            save_undo();
            for (int i = 0; i < total_count; i++) {
                if (E.lines[E.cy].len > 0 && E.cx < E.lines[E.cy].len) {
                    line_delete_char(&E.lines[E.cy], E.cx);
                }
            }
            E.curr_action.type = ACT_INSERT;
            E.curr_action.op = 'i';
            E.curr_action.count = 1;
            E.mode = MODE_INSERT;
            break;
        case 'S':
            apply_operator_on_range('c', E.cy, 0, min(E.num_lines - 1, E.cy + total_count - 1), 0, true);
            E.curr_action.type = ACT_CHANGE_MOTION;
            E.curr_action.op = 'c';
            E.curr_action.count = total_count;
            break;
        case 'C':
            apply_operator_on_range('c', E.cy, E.cx, E.cy, E.lines[E.cy].len, false);
            E.curr_action.type = ACT_CHANGE_MOTION;
            E.curr_action.op = 'c';
            E.curr_action.motion = '$';
            E.curr_action.motion_count = 1;
            E.curr_action.count = 1;
            break;
        case 'D':
            apply_operator_on_range('d', E.cy, E.cx, E.cy, E.lines[E.cy].len, false);
            E.curr_action.type = ACT_DIRECT;
            E.curr_action.op = 'D';
            finalize_repeat_action();
            break;
        case 'Y':
            apply_operator_on_range('y', E.cy, 0, min(E.num_lines - 1, E.cy + total_count - 1), 0, true);
            break;
        case 'r': {
            int rep = editor_read_key();
            if (isprint(rep) && E.lines[E.cy].len > 0) {
                save_undo();
                for (int i = 0; i < total_count; i++) {
                    if (E.cx + i < E.lines[E.cy].len) {
                        E.lines[E.cy].chars[E.cx + i] = (char)rep;
                        E.dirty = true;
                    }
                }
                E.cx = min(max(0, E.lines[E.cy].len - 1), E.cx + total_count - 1);
                E.curr_action.type = ACT_DIRECT;
                E.curr_action.op = 'r';
                E.curr_action.count = total_count;
                E.curr_action.arg_char = (char)rep;
                finalize_repeat_action();
            }
            break;
        }
        case 'J':
            save_undo();
            for (int i = 0; i < total_count; i++) join_two_rows(E.cy);
            E.curr_action.type = ACT_DIRECT;
            E.curr_action.op = 'J';
            E.curr_action.count = total_count;
            finalize_repeat_action();
            break;
        case '~':
            save_undo();
            for (int i = 0; i < total_count; i++) {
                if (E.cx < E.lines[E.cy].len) {
                    char ch = E.lines[E.cy].chars[E.cx];
                    if (isupper((unsigned char)ch)) ch = (char)tolower((unsigned char)ch);
                    else if (islower((unsigned char)ch)) ch = (char)toupper((unsigned char)ch);
                    E.lines[E.cy].chars[E.cx] = ch;
                    E.dirty = true;
                    if (E.cx < E.lines[E.cy].len - 1) E.cx++;
                }
            }
            E.curr_action.type = ACT_DIRECT;
            E.curr_action.op = '~';
            E.curr_action.count = total_count;
            finalize_repeat_action();
            break;
        case 'p':
            do_paste(true, total_count);
            E.curr_action.type = ACT_DIRECT;
            E.curr_action.op = 'p';
            E.curr_action.count = total_count;
            finalize_repeat_action();
            break;
        case 'P':
            do_paste(false, total_count);
            E.curr_action.type = ACT_DIRECT;
            E.curr_action.op = 'P';
            E.curr_action.count = total_count;
            finalize_repeat_action();
            break;

        case 'u': apply_undo(); break;
        case 'U': restore_u_line(); break;

        case ':':
            E.mode = MODE_COMMAND;
            E.cmd_buf[0] = '\0';
            E.cmd_len = 0;
            E.hist_idx = E.cmd_hist.count;
            break;
        case '/':
            E.mode = MODE_SEARCH;
            E.search_forward = true;
            E.cmd_buf[0] = '\0';
            E.cmd_len = 0;
            E.hist_idx = E.search_hist.count;
            s_search_orig_cy = E.cy;
            s_search_orig_cx = E.cx;
            strncpy(s_search_saved_last, E.last_search, sizeof(s_search_saved_last) - 1);
            s_search_saved_last[sizeof(s_search_saved_last) - 1] = '\0';
            break;
        case '?':
            E.mode = MODE_SEARCH;
            E.search_forward = false;
            E.cmd_buf[0] = '\0';
            E.cmd_len = 0;
            E.hist_idx = E.search_hist.count;
            s_search_orig_cy = E.cy;
            s_search_orig_cx = E.cx;
            strncpy(s_search_saved_last, E.last_search, sizeof(s_search_saved_last) - 1);
            s_search_saved_last[sizeof(s_search_saved_last) - 1] = '\0';
            break;
        case 'n': E.opt.nohl_active = false; editor_search(E.search_forward); break;
        case 'N': E.opt.nohl_active = false; editor_search(!E.search_forward); break;

        case 29: { /* Ctrl-] (CTags Jump) */
            char sym[128];
            if (get_word_under_cursor(sym, sizeof(sym))) {
                tags_jump(sym);
            } else {
                set_status("No identifier under cursor");
            }
            break;
        }
        case 20: { /* Ctrl-T (CTags Pop) */
            tags_pop();
            break;
        }
        case 23: { /* Ctrl-W (Window navigation & management) */
            int wcmd = editor_read_key();
            if (wcmd == 's' || wcmd == 'S') {
                window_split(false);
            } else if (wcmd == 'v' || wcmd == 'V') {
                window_split(true);
            } else if (wcmd == 'c' || wcmd == 'q') {
                window_close_current();
            } else if (wcmd == 'o') {
                while (E.num_windows > 1) window_close_current();
            } else if (wcmd == 'w' || wcmd == 'W') {
                window_next();
            } else if (wcmd == 'p') {
                window_prev();
            } else if (wcmd == 'h' || wcmd == KEY_ARROW_LEFT) {
                window_move_dir('h');
            } else if (wcmd == 'j' || wcmd == KEY_ARROW_DOWN) {
                window_move_dir('j');
            } else if (wcmd == 'k' || wcmd == KEY_ARROW_UP) {
                window_move_dir('k');
            } else if (wcmd == 'l' || wcmd == KEY_ARROW_RIGHT) {
                window_move_dir('l');
            } else if (wcmd == '=') {
                window_balance();
            }
            break;
        }

        case 'q': {
            int r = term_read_key();
            if ((r >= 'a' && r <= 'z') || (r >= 'A' && r <= 'Z')) {
                E.recording_macro = true;
                E.macro_reg = (char)tolower(r);
                s_macro_record_len = 0;
                set_status("recording @%c", E.macro_reg);
            }
            break;
        }
        case '@': {
            int r = term_read_key();
            if (r == '@') {
                r = E.last_macro_reg;
            }
            if ((r >= 'a' && r <= 'z') || (r >= 'A' && r <= 'Z')) {
                char reg_char = (char)tolower(r);
                Register *reg = reg_get(reg_char);
                if (reg && reg->data && reg->len > 0) {
                    E.last_macro_reg = reg_char;
                    for (int t = 0; t < total_count; t++) {
                        for (int i = 0; i < reg->len; i++) {
                            term_push_key((unsigned char)reg->data[i]);
                        }
                    }
                } else {
                    set_status("Register '%c' empty", reg_char);
                }
            }
            break;
        }

        case 'v':
            E.mode = MODE_VISUAL;
            E.visual_anchor_y = E.cy;
            E.visual_anchor_x = E.cx;
            break;
        case 'V':
            E.mode = MODE_VISUAL_LINE;
            E.visual_anchor_y = E.cy;
            E.visual_anchor_x = 0;
            break;
        case 17: /* Ctrl-Q */
        case 22: /* Ctrl-V */
            E.mode = MODE_VISUAL_BLOCK;
            E.visual_anchor_y = E.cy;
            E.visual_anchor_x = E.cx;
            break;
        case 18: /* Ctrl-R (Redo) */
            apply_redo();
            break;

        case 'Q': E.mode = MODE_EX; break;

        case 'Z': {
            int z2 = editor_read_key();
            if (z2 == 'Z') {
                if (!E.dirty || editor_save(NULL, false)) exit(0);
            } else if (z2 == 'Q') {
                exit(0);
            }
            break;
        }

        default: {
            if (c == 'g') {
                int next = editor_read_key();
                if (next == 'g') {
                    E.cy = 0;
                    E.cx = 0;
                } else if (next == 't') {
                    tab_next();
                } else if (next == 'T') {
                    tab_prev();
                }
                break;
            }
            MotionResult m = evaluate_motion(c, total_count);
            if (m.valid) {
                E.cy = m.y;
                E.cx = m.x;
                if (E.cy < E.num_lines && E.cx >= E.lines[E.cy].len && E.lines[E.cy].len > 0) {
                    E.cx = E.lines[E.cy].len - 1;
                }
            }
            break;
        }
    }

    E.count1 = 0;
    E.count2 = 0;
    backup_u_line();
}

void init_editor(void) {
    E.cx = 0;
    E.cy = 0;
    E.screen_x = 0;
    E.row_off = 0;
    E.col_off = 0;
    E.num_lines = 0;
    E.lines = NULL;
    E.filename = NULL;
    E.dirty = false;
    E.is_crlf = false;
    E.mode = MODE_NORMAL;
    E.cmd_len = 0;
    E.status_msg[0] = '\0';
    E.status_msg_time = 0;
    E.last_search[0] = '\0';
    E.search_forward = true;
    E.inline_char = 0;
    E.inline_cmd = 0;

    E.opt.number = false;
    E.opt.ignorecase = false;
    E.opt.showmode = true;
    E.opt.ruler = false;
    E.opt.autoindent = true;
    E.opt.hlsearch = false;
    E.opt.nohl_active = false;
    E.opt.syntax = true;
    E.opt.compatible = false;
    E.opt.incsearch = true;
    E.opt.wrap = false;
    E.opt.mouse = true;
    strcpy(E.opt.colorscheme, "dark");
    E.opt.tabstop = 4;
    E.opt.shiftwidth = 4;
    E.tag_stack_top = 0;

    E.visual_anchor_y = 0;
    E.visual_anchor_x = 0;

    E.recording_macro = false;
    E.macro_reg = 0;
    E.last_macro_reg = 'a';
    E.macro_queue.head = 0;
    E.macro_queue.tail = 0;

    memset(&E.cmd_hist, 0, sizeof(E.cmd_hist));
    memset(&E.search_hist, 0, sizeof(E.search_hist));
    E.hist_idx = 0;

    E.args_files = NULL;
    E.num_args = 0;
    E.cur_arg = 0;

    memset(&E.reg_unnamed, 0, sizeof(Register));
    memset(E.reg_named, 0, sizeof(E.reg_named));
    memset(E.reg_numbered, 0, sizeof(E.reg_numbered));
    memset(&E.reg_clipboard, 0, sizeof(Register));
    E.active_reg = 0;

    memset(E.marks, 0, sizeof(E.marks));
    memset(&E.mark_prev, 0, sizeof(Mark));

    E.undo_stack.top = 0;
    E.redo_stack.top = 0;
    E.u_line_backup = NULL;
    E.u_line_cy = -1;
    memset(&E.last_action, 0, sizeof(RepeatRecord));
    memset(&E.curr_action, 0, sizeof(RepeatRecord));

    E.count1 = 0;
    E.count2 = 0;
    E.pending_op = 0;
    E.reading_count2 = false;
    E.help_scroll = 0;

    buffer_init();
    window_init();
    diff_init();
    quickfix_init();
    tab_init();
    fold_init();
    term_enable_raw();
}

int main(int argc, char *argv[]) {
    init_editor();
    atexit(term_disable_raw);
    atexit(editor_cleanup);

    load_virc();

    /* Detect if launched as "view" or "view.exe" */
    bool flag_readonly = false;
    bool flag_diff = false;
    const char *session_file = NULL;

    if (argv[0]) {
        const char *base = strrchr(argv[0], '\\');
        if (!base) base = strrchr(argv[0], '/');
        base = base ? base + 1 : argv[0];
        if (_stricmp(base, "view") == 0 || _stricmp(base, "view.exe") == 0) {
            flag_readonly = true;
        }
    }

    int initial_line = 0;
    const char *initial_search = NULL;
    static char *file_args[MAX_BUFFERS];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '+') {
            if (argv[i][1] == '/') {
                initial_search = &argv[i][2];
            } else if (isdigit((unsigned char)argv[i][1])) {
                initial_line = atoi(&argv[i][1]);
            } else if (argv[i][1] == '\0') {
                initial_line = -1; /* jump to end of file */
            }
        } else if (strcmp(argv[i], "-R") == 0 || strcmp(argv[i], "-r") == 0) {
            flag_readonly = true;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-diff") == 0) {
            flag_diff = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                session_file = argv[++i];
            } else {
                session_file = "Session.vim";
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            term_disable_raw();
            printf("%s\n", VI_VERSION);
            exit(0);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            term_disable_raw();
            printf("Usage: vi [-R] [-d file1 file2] [-S [session]] [+line | +/pattern] [file ...]\n");
            printf("       view [+line | +/pattern] [file ...]\n");
            exit(0);
        } else {
            if (file_count < MAX_BUFFERS) {
                file_args[file_count++] = argv[i];
            }
        }
    }

    if (session_file) {
        session_load(session_file);
    } else if (flag_diff && file_count >= 2) {
        editor_open(file_args[0]);
        diff_split(file_args[1]);
    } else if (file_count > 0) {
        if (is_directory(file_args[0])) {
            explorer_open(file_args[0]);
        } else {
            E.args_files = file_args;
            E.num_args = file_count;
            E.cur_arg = 0;
            editor_open(file_args[0]);
            if (flag_readonly) {
                E.readonly = true;
                if (E.cur_buf_idx >= 0 && E.cur_buf_idx < E.num_buffers) {
                    E.buffers[E.cur_buf_idx].readonly = true;
                }
            }
            for (int i = 1; i < file_count; i++) {
                buffer_add(file_args[i]);
                if (flag_readonly && i < E.num_buffers) {
                    E.buffers[i].readonly = true;
                }
            }

            if (initial_line > 0) {
                E.cy = min(E.num_lines - 1, max(0, initial_line - 1));
                E.cx = 0;
            } else if (initial_line == -1) {
                E.cy = max(0, E.num_lines - 1);
                E.cx = 0;
            }

            if (initial_search && *initial_search) {
                strncpy(E.last_search, initial_search, sizeof(E.last_search) - 1);
                E.last_search[sizeof(E.last_search) - 1] = '\0';
                E.cy = 0;
                E.cx = -1;
                editor_search(true);
            }
        }
    } else {
        insert_row(0, "", 0);
        E.buffers[0].id = E.next_buffer_id++;
        E.buffers[0].filename = NULL;
        E.buffers[0].loaded = true;
        if (flag_readonly) {
            E.readonly = true;
            E.buffers[0].readonly = true;
        }
        E.num_buffers = 1;
        E.cur_buf_idx = 0;
        buffer_save_current_state();
        set_status("Welcome to Vi v5.0.0-2026 Type :help or <F1> for help.");
    }

    while (1) {
        editor_refresh_screen();
        process_keypress();
    }

    return 0;
}
