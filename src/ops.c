#include "vi.h"
#include "ops.h"
#include "buffer.h"
#include "undo.h"
#include "reg.h"
#include "motion.h"
#include "screen.h"

void apply_operator_on_range(char op, int y1, int x1, int y2, int x2, bool linewise) {
    if (E.num_lines == 0) return;
    if (y1 < 0) y1 = 0;
    if (y2 >= E.num_lines) y2 = E.num_lines - 1;
    if (y1 > y2 || (y1 == y2 && x1 > x2)) {
        int ty = y1; y1 = y2; y2 = ty;
        int tx = x1; x1 = x2; x2 = tx;
    }
    if (x1 < 0) x1 = 0;
    if (x1 > E.lines[y1].len) x1 = E.lines[y1].len;
    if (x2 < 0) x2 = 0;
    if (x2 > E.lines[y2].len) x2 = E.lines[y2].len;

    if (op == '>' || op == '<') {
        linewise = true;
    }

    if (linewise) {
        if (op == 'd' || op == 'c' || op == 'y') {
            int total_len = 0;
            for (int i = y1; i <= y2; i++) total_len += E.lines[i].len + 1;
            char *reg_buf = (char *)malloc(total_len + 1);
            int ptr = 0;
            for (int i = y1; i <= y2; i++) {
                memcpy(&reg_buf[ptr], E.lines[i].chars, E.lines[i].len);
                ptr += E.lines[i].len;
                reg_buf[ptr++] = '\n';
            }
            reg_buf[ptr] = '\0';

            reg_set(E.active_reg, reg_buf, ptr, VI_REG_LINE);
            if (op == 'd' && E.active_reg == 0) push_numbered_reg(reg_buf, ptr);
            free(reg_buf);
        }

        if (op == 'd' || op == 'c') {
            save_undo();
            int count_to_del = y2 - y1 + 1;
            for (int i = 0; i < count_to_del; i++) delete_row(y1);
            if (E.num_lines == 0) insert_row(0, "", 0);
            E.cy = min(y1, E.num_lines - 1);
            E.cx = 0;
            if (op == 'c') {
                insert_row(E.cy, "", 0);
                E.mode = MODE_INSERT;
            }
        } else if (op == '>') {
            save_undo();
            for (int i = y1; i <= y2; i++) {
                for (int s = 0; s < E.opt.shiftwidth; s++) {
                    line_insert_char(&E.lines[i], 0, ' ');
                }
            }
            E.cy = y1;
            E.cx = 0;
        } else if (op == '<') {
            save_undo();
            for (int i = y1; i <= y2; i++) {
                for (int s = 0; s < E.opt.shiftwidth; s++) {
                    if (E.lines[i].len > 0 && isspace((unsigned char)E.lines[i].chars[0])) {
                        line_delete_char(&E.lines[i], 0);
                    }
                }
            }
            E.cy = y1;
            E.cx = 0;
        } else if (op == '~') {
            save_undo();
            for (int i = y1; i <= y2; i++) {
                for (int j = 0; j < E.lines[i].len; j++) {
                    char ch = E.lines[i].chars[j];
                    if (isupper((unsigned char)ch)) E.lines[i].chars[j] = (char)tolower((unsigned char)ch);
                    else if (islower((unsigned char)ch)) E.lines[i].chars[j] = (char)toupper((unsigned char)ch);
                }
            }
            E.dirty = true;
            E.cy = y1;
            E.cx = 0;
        } else if (op == 'y') {
            int lines = y2 - y1 + 1;
            if (lines > 1) set_status("%d lines yanked", lines);
            E.cy = y1;
            E.cx = x1;
        }
    } else if (op == '~') {
        save_undo();
        for (int r = y1; r <= y2 && r < E.num_lines; r++) {
            int start_x = (r == y1) ? x1 : 0;
            int end_x = (r == y2) ? x2 : E.lines[r].len;
            for (int j = start_x; j < end_x && j < E.lines[r].len; j++) {
                char ch = E.lines[r].chars[j];
                if (isupper((unsigned char)ch)) E.lines[r].chars[j] = (char)tolower((unsigned char)ch);
                else if (islower((unsigned char)ch)) E.lines[r].chars[j] = (char)toupper((unsigned char)ch);
            }
        }
        E.dirty = true;
        E.cy = y1;
        E.cx = x1;
    } else {
        if (y1 == y2) {
            int slice_len = x2 - x1;
            if (slice_len > 0) {
                reg_set(E.active_reg, &E.lines[y1].chars[x1], slice_len, VI_REG_CHAR);
                if (op == 'd' || op == 'c') {
                    save_undo();
                    line_delete_range(&E.lines[y1], x1, slice_len);
                    E.cx = x1;
                    if (E.cx >= E.lines[y1].len && E.cx > 0) E.cx = E.lines[y1].len - 1;
                    if (op == 'c') E.mode = MODE_INSERT;
                }
            }
        } else {
            if (op == 'd' || op == 'c') save_undo();
            int cap = 1024, len = 0;
            char *buf = (char *)malloc(cap);

            int first_rem = E.lines[y1].len - x1;
            memcpy(buf, &E.lines[y1].chars[x1], first_rem);
            len += first_rem;
            buf[len++] = '\n';

            for (int i = y1 + 1; i < y2; i++) {
                while (len + E.lines[i].len + 2 > cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                memcpy(&buf[len], E.lines[i].chars, E.lines[i].len);
                len += E.lines[i].len;
                buf[len++] = '\n';
            }

            while (len + x2 + 2 > cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            memcpy(&buf[len], E.lines[y2].chars, x2);
            len += x2;
            buf[len] = '\0';

            reg_set(E.active_reg, buf, len, VI_REG_CHAR);
            free(buf);

            if (op == 'd' || op == 'c') {
                int y2_suffix_len = E.lines[y2].len - x2;
                char *suffix = _strdup(&E.lines[y2].chars[x2]);
                E.lines[y1].len = x1;
                E.lines[y1].chars[x1] = '\0';
                line_insert_str(&E.lines[y1], x1, suffix, y2_suffix_len);
                free(suffix);

                for (int i = y1 + 1; i <= y2; i++) delete_row(y1 + 1);
                E.cy = y1;
                E.cx = x1;
                if (op == 'c') E.mode = MODE_INSERT;
            }
        }
    }

    E.active_reg = 0;
}

void record_insert_char(char c) {
    if (E.curr_action.inserted_len + 2 > E.curr_action.inserted_cap) {
        E.curr_action.inserted_cap = (E.curr_action.inserted_cap + 32) * 2;
        E.curr_action.inserted = (char *)realloc(E.curr_action.inserted, E.curr_action.inserted_cap);
    }
    E.curr_action.inserted[E.curr_action.inserted_len++] = c;
    E.curr_action.inserted[E.curr_action.inserted_len] = '\0';
}

void finalize_repeat_action(void) {
    free(E.last_action.inserted);
    E.last_action = E.curr_action;
    E.last_action.valid = true;
    memset(&E.curr_action, 0, sizeof(E.curr_action));
}

void repeat_last_action(int override_count) {
    if (!E.last_action.valid) {
        set_status("No previous command to repeat");
        return;
    }

    RepeatRecord act = E.last_action;
    int count = override_count > 0 ? override_count : (act.count > 0 ? act.count : 1);

    if (act.type == ACT_DIRECT) {
        switch (act.op) {
            case 'x':
                save_undo();
                for (int i = 0; i < count; i++) {
                    if (E.lines[E.cy].len > 0 && E.cx < E.lines[E.cy].len) {
                        line_delete_char(&E.lines[E.cy], E.cx);
                    }
                }
                if (E.cx >= E.lines[E.cy].len && E.cx > 0) E.cx = E.lines[E.cy].len - 1;
                break;
            case 'X':
                save_undo();
                for (int i = 0; i < count; i++) {
                    if (E.cx > 0) {
                        line_delete_char(&E.lines[E.cy], E.cx - 1);
                        E.cx--;
                    }
                }
                break;
            case 'r':
                save_undo();
                for (int i = 0; i < count; i++) {
                    if (E.cx + i < E.lines[E.cy].len) {
                        E.lines[E.cy].chars[E.cx + i] = act.arg_char;
                        E.dirty = true;
                    }
                }
                E.cx = min(max(0, E.lines[E.cy].len - 1), E.cx + count - 1);
                break;
            case 'J':
                save_undo();
                for (int i = 0; i < count; i++) join_two_rows(E.cy);
                break;
            case '~':
                save_undo();
                for (int i = 0; i < count; i++) {
                    if (E.cx < E.lines[E.cy].len) {
                        char c = E.lines[E.cy].chars[E.cx];
                        if (isupper((unsigned char)c)) c = (char)tolower((unsigned char)c);
                        else if (islower((unsigned char)c)) c = (char)toupper((unsigned char)c);
                        E.lines[E.cy].chars[E.cx] = c;
                        E.dirty = true;
                        if (E.cx < E.lines[E.cy].len - 1) E.cx++;
                    }
                }
                break;
            case 'p': do_paste(true, count); break;
            case 'P': do_paste(false, count); break;
            case 'D':
                apply_operator_on_range('d', E.cy, E.cx, E.cy, E.lines[E.cy].len, false);
                break;
            case 'd':
                apply_operator_on_range('d', E.cy, 0, min(E.num_lines - 1, E.cy + count - 1), 0, true);
                break;
            case '>':
                apply_operator_on_range('>', E.cy, 0, min(E.num_lines - 1, E.cy + count - 1), 0, true);
                break;
            case '<':
                apply_operator_on_range('<', E.cy, 0, min(E.num_lines - 1, E.cy + count - 1), 0, true);
                break;
        }
    } else if (act.type == ACT_INSERT || act.type == ACT_CHANGE_MOTION) {
        save_undo();
        if (act.type == ACT_CHANGE_MOTION) {
            if (act.motion == 0) {
                apply_operator_on_range('c', E.cy, 0, min(E.num_lines - 1, E.cy + count - 1), 0, true);
            } else if (act.motion == 'i' || act.motion == 'a') {
                int y1, x1, y2, x2;
                if (evaluate_text_object(act.motion, act.arg_char, &y1, &x1, &y2, &x2)) {
                    apply_operator_on_range('c', y1, x1, y2, x2 + 1, act.arg_char == 'p');
                }
            } else {
                if (act.motion == 'f' || act.motion == 'F' || act.motion == 't' || act.motion == 'T') {
                    E.inline_cmd = act.motion;
                    E.inline_char = act.arg_char;
                }
                MotionResult m = evaluate_motion(act.motion, act.motion_count > 0 ? act.motion_count : count);
                if (m.valid) {
                    int y1 = E.cy, x1 = E.cx;
                    int y2 = m.y, x2 = m.x;
                    if (!m.linewise && m.inclusive) {
                        if (y1 < y2 || (y1 == y2 && x1 <= x2)) x2++;
                        else x1++;
                    }
                    apply_operator_on_range('c', y1, x1, y2, x2, m.linewise);
                }
            }
            E.mode = MODE_NORMAL;
        } else if (act.op == 'o') {
            insert_row(E.cy + 1, "", 0);
            E.cy++;
            E.cx = 0;
        } else if (act.op == 'O') {
            insert_row(E.cy, "", 0);
            E.cx = 0;
        } else if (act.op == 'a') {
            if (E.lines[E.cy].len > 0) E.cx++;
        } else if (act.op == 'A') {
            E.cx = E.lines[E.cy].len;
        } else if (act.op == 'I') {
            E.cx = 0;
            while (E.cx < E.lines[E.cy].len && isspace((unsigned char)E.lines[E.cy].chars[E.cx])) E.cx++;
        }

        int ins_repeats = (act.type == ACT_CHANGE_MOTION) ? 1 : count;
        if (act.inserted && act.inserted_len > 0) {
            for (int n = 0; n < ins_repeats; n++) {
                const char *p = act.inserted;
                while (*p) {
                    if (*p == '\r' || *p == '\n') {
                        Line *row = &E.lines[E.cy];
                        insert_row(E.cy + 1, &row->chars[E.cx], row->len - E.cx);
                        row = &E.lines[E.cy];
                        row->len = E.cx;
                        row->chars[row->len] = '\0';
                        E.cy++;
                        E.cx = 0;
                    } else {
                        line_insert_char(&E.lines[E.cy], E.cx++, *p);
                    }
                    p++;
                }
            }
            if (E.cx > 0) E.cx--;
        }
    } else if (act.type == ACT_DELETE_MOTION) {
        if (act.motion == 'i' || act.motion == 'a') {
            int y1, x1, y2, x2;
            if (evaluate_text_object(act.motion, act.arg_char, &y1, &x1, &y2, &x2)) {
                apply_operator_on_range(act.op ? act.op : 'd', y1, x1, y2, x2 + 1, act.arg_char == 'p');
            }
        } else {
            if (act.motion == 'f' || act.motion == 'F' || act.motion == 't' || act.motion == 'T') {
                E.inline_cmd = act.motion;
                E.inline_char = act.arg_char;
            }
            MotionResult m = evaluate_motion(act.motion, override_count > 0 ? override_count : (act.motion_count > 0 ? act.motion_count : count));
            if (m.valid) {
                int y1 = E.cy, x1 = E.cx;
                int y2 = m.y, x2 = m.x;
                if (!m.linewise && m.inclusive) {
                    if (y1 < y2 || (y1 == y2 && x1 <= x2)) x2++;
                    else x1++;
                }
                apply_operator_on_range(act.op ? act.op : 'd', y1, x1, y2, x2, m.linewise);
            }
        }
    }
}

void apply_block_operator(char op) {
    if (E.num_lines == 0) return;
    int y1 = min(E.visual_anchor_y, E.cy);
    int y2 = max(E.visual_anchor_y, E.cy);
    int x1 = min(E.visual_anchor_x, E.cx);
    int x2 = max(E.visual_anchor_x, E.cx);

    if (y1 < 0) y1 = 0;
    if (y2 >= E.num_lines) y2 = E.num_lines - 1;
    if (x1 < 0) x1 = 0;

    int num_rows = y2 - y1 + 1;
    int block_w = x2 - x1 + 1;

    if (op == '~' || op == '>' || op == '<') {
        save_undo();
        for (int r = y1; r <= y2; r++) {
            Line *line = &E.lines[r];
            if (op == '~') {
                for (int c = x1; c <= x2 && c < line->len; c++) {
                    char ch = line->chars[c];
                    if (isupper((unsigned char)ch)) line->chars[c] = (char)tolower((unsigned char)ch);
                    else if (islower((unsigned char)ch)) line->chars[c] = (char)toupper((unsigned char)ch);
                }
            } else if (op == '>' && x1 < line->len) {
                for (int s = 0; s < E.opt.shiftwidth; s++) line_insert_char(line, x1, ' ');
            } else if (op == '<' && x1 < line->len) {
                for (int s = 0; s < E.opt.shiftwidth; s++) {
                    if (x1 < line->len && isspace((unsigned char)line->chars[x1])) line_delete_char(line, x1);
                }
            }
        }
        E.dirty = true;
        E.cy = y1;
        E.cx = x1;
        E.mode = MODE_NORMAL;
        return;
    }

    /* Collect block text for register */
    int total_chars = num_rows * (block_w + 1);
    char *yank_buf = (char *)malloc(total_chars + 1);
    int yptr = 0;

    for (int r = y1; r <= y2; r++) {
        Line *line = &E.lines[r];
        for (int c = x1; c <= x2; c++) {
            if (c < line->len) {
                yank_buf[yptr++] = line->chars[c];
            } else {
                yank_buf[yptr++] = ' ';
            }
        }
        yank_buf[yptr++] = '\n';
    }
    yank_buf[yptr] = '\0';
    reg_set(E.active_reg, yank_buf, yptr, VI_REG_BLOCK);
    free(yank_buf);

    if (op == 'y') {
        E.cy = y1;
        E.cx = x1;
        E.mode = MODE_NORMAL;
        set_status("%d lines block yanked", num_rows);
        return;
    }

    if (op == 'd' || op == 'x' || op == 'c') {
        save_undo();
        for (int r = y1; r <= y2; r++) {
            Line *line = &E.lines[r];
            if (x1 < line->len) {
                int del_count = min(block_w, line->len - x1);
                line_delete_range(line, x1, del_count);
            }
        }
        E.cy = y1;
        E.cx = x1;
        if (op == 'c') {
            E.mode = MODE_INSERT;
            E.curr_action.type = ACT_INSERT;
            E.curr_action.count = num_rows;
            E.curr_action.motion = 'B';
            E.curr_action.arg_char = (char)x1;
        } else {
            E.mode = MODE_NORMAL;
            set_status("%d lines block deleted", num_rows);
        }
    }
}
