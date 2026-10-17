#include "vi.h"
#include "motion.h"
#include "fold.h"

static inline bool is_word_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static inline bool is_blank(char c) {
    return isspace((unsigned char)c);
}

MotionResult motion_word_forward(int count, bool big_word) {
    MotionResult res = { E.cy, E.cx, false, false, true };
    if (E.num_lines == 0) return res;
    for (int n = 0; n < count; n++) {
        if (res.y >= E.num_lines) break;
        Line *cur = &E.lines[res.y];
        if (res.x >= cur->len) {
            if (res.y < E.num_lines - 1) {
                res.y++;
                res.x = 0;
                cur = &E.lines[res.y];
            } else break;
        }

        char c = cur->chars[res.x];
        if (!big_word) {
            if (is_word_char(c)) {
                while (res.x < cur->len && is_word_char(cur->chars[res.x])) res.x++;
            } else if (!is_blank(c)) {
                while (res.x < cur->len && !is_word_char(cur->chars[res.x]) && !is_blank(cur->chars[res.x])) res.x++;
            }
        } else {
            while (res.x < cur->len && !is_blank(cur->chars[res.x])) res.x++;
        }

        while (res.x < cur->len && is_blank(cur->chars[res.x])) res.x++;
        if (res.x >= cur->len && res.y < E.num_lines - 1) {
            res.y++;
            res.x = 0;
            cur = &E.lines[res.y];
            while (res.x < cur->len && is_blank(cur->chars[res.x])) res.x++;
        }
    }
    return res;
}

MotionResult motion_word_backward(int count, bool big_word) {
    MotionResult res = { E.cy, E.cx, false, false, true };
    if (E.num_lines == 0) return res;
    for (int n = 0; n < count; n++) {
        if (res.x == 0 && res.y > 0) {
            res.y--;
            res.x = E.lines[res.y].len;
        }
        if (res.y < 0) break;
        Line *cur = &E.lines[res.y];

        while (res.x > 0 && is_blank(cur->chars[res.x - 1])) res.x--;
        if (res.x == 0 && res.y > 0) {
            res.y--;
            cur = &E.lines[res.y];
            res.x = cur->len;
            while (res.x > 0 && is_blank(cur->chars[res.x - 1])) res.x--;
        }

        if (res.x > 0) {
            char prev = cur->chars[res.x - 1];
            if (!big_word) {
                if (is_word_char(prev)) {
                    while (res.x > 0 && is_word_char(cur->chars[res.x - 1])) res.x--;
                } else {
                    while (res.x > 0 && !is_word_char(cur->chars[res.x - 1]) && !is_blank(cur->chars[res.x - 1])) res.x--;
                }
            } else {
                while (res.x > 0 && !is_blank(cur->chars[res.x - 1])) res.x--;
            }
        }
    }
    return res;
}

MotionResult motion_word_end(int count, bool big_word) {
    MotionResult res = { E.cy, E.cx, false, true, true };
    if (E.num_lines == 0) return res;
    for (int n = 0; n < count; n++) {
        if (res.y >= E.num_lines) break;
        Line *cur = &E.lines[res.y];
        if (res.x + 1 < cur->len) res.x++;
        else if (res.y < E.num_lines - 1) {
            res.y++;
            res.x = 0;
            cur = &E.lines[res.y];
        }

        while (res.x < cur->len && is_blank(cur->chars[res.x])) res.x++;
        if (res.x >= cur->len && res.y < E.num_lines - 1) {
            res.y++;
            res.x = 0;
            cur = &E.lines[res.y];
            while (res.x < cur->len && is_blank(cur->chars[res.x])) res.x++;
        }

        char c = cur->chars[res.x];
        if (!big_word) {
            if (is_word_char(c)) {
                while (res.x + 1 < cur->len && is_word_char(cur->chars[res.x + 1])) res.x++;
            } else {
                while (res.x + 1 < cur->len && !is_word_char(cur->chars[res.x + 1]) && !is_blank(cur->chars[res.x + 1])) res.x++;
            }
        } else {
            while (res.x + 1 < cur->len && !is_blank(cur->chars[res.x + 1])) res.x++;
        }
    }
    return res;
}

MotionResult motion_inline_find(char cmd, char target, int count) {
    MotionResult res = { E.cy, E.cx, false, false, false };
    if (E.cy >= E.num_lines) return res;
    Line *cur = &E.lines[E.cy];
    int step = (cmd == 'f' || cmd == 't') ? 1 : -1;
    int pos = E.cx;

    for (int n = 0; n < count; n++) {
        pos += step;
        while (pos >= 0 && pos < cur->len) {
            if (cur->chars[pos] == target) break;
            pos += step;
        }
        if (pos < 0 || pos >= cur->len) return res;
    }

    if (cmd == 't') pos--;
    if (cmd == 'T') pos++;
    res.x = pos;
    res.inclusive = (cmd == 'f' || cmd == 't');
    res.valid = true;
    return res;
}

MotionResult motion_bracket_match(void) {
    MotionResult res = { E.cy, E.cx, false, true, false };
    if (E.cy >= E.num_lines) return res;
    Line *cur = &E.lines[E.cy];

    int find_x = E.cx;
    char open_ch = 0, close_ch = 0;
    int dir = 1;

    while (find_x < cur->len) {
        char ch = cur->chars[find_x];
        if (ch == '(') { open_ch = '('; close_ch = ')'; dir = 1; break; }
        if (ch == ')') { open_ch = ')'; close_ch = '('; dir = -1; break; }
        if (ch == '[') { open_ch = '['; close_ch = ']'; dir = 1; break; }
        if (ch == ']') { open_ch = ']'; close_ch = '['; dir = -1; break; }
        if (ch == '{') { open_ch = '{'; close_ch = '}'; dir = 1; break; }
        if (ch == '}') { open_ch = '}'; close_ch = '{'; dir = -1; break; }
        find_x++;
    }
    if (!open_ch) return res;

    int depth = 1;
    int y = E.cy;
    int x = find_x + dir;

    while (y >= 0 && y < E.num_lines) {
        Line *l = &E.lines[y];
        while (x >= 0 && x < l->len) {
            if (l->chars[x] == open_ch) depth++;
            else if (l->chars[x] == close_ch) {
                depth--;
                if (depth == 0) {
                    res.y = y;
                    res.x = x;
                    res.valid = true;
                    return res;
                }
            }
            x += dir;
        }
        y += dir;
        if (y >= 0 && y < E.num_lines) {
            x = (dir == 1) ? 0 : E.lines[y].len - 1;
        }
    }
    return res;
}

MotionResult evaluate_motion(int key, int count) {
    MotionResult res = { E.cy, E.cx, false, false, false };
    if (E.num_lines == 0) return res;
    if (count <= 0) count = 1;

    switch (key) {
        case 'h':
            res.x = max(0, E.cx - count);
            res.valid = true;
            break;
        case 'l':
            if (E.cy < E.num_lines) {
                res.x = min(max(0, E.lines[E.cy].len - 1), E.cx + count);
                res.valid = true;
            }
            break;
        case 'j':
            for (int n = 0; n < count; n++) {
                Fold *f = fold_find_start(res.y);
                if (f) {
                    res.y = min(E.num_lines - 1, f->end_y + 1);
                } else {
                    res.y = min(E.num_lines - 1, res.y + 1);
                }
            }
            res.linewise = true;
            res.valid = true;
            break;
        case 'k':
            for (int n = 0; n < count; n++) {
                res.y = max(0, res.y - 1);
                if (fold_is_line_hidden(res.y)) {
                    Fold *f = fold_find_at(res.y);
                    if (f) res.y = f->start_y;
                }
            }
            res.linewise = true;
            res.valid = true;
            break;
        case '0':
            res.x = 0;
            res.valid = true;
            break;
        case '^':
            res.x = 0;
            if (E.cy < E.num_lines) {
                while (res.x < E.lines[E.cy].len && is_blank(E.lines[E.cy].chars[res.x])) res.x++;
            }
            res.valid = true;
            break;
        case '$':
            if (E.cy < E.num_lines) {
                res.y = min(E.num_lines - 1, E.cy + count - 1);
                res.x = max(0, E.lines[res.y].len - 1);
                res.inclusive = true;
                res.valid = true;
            }
            break;
        case 'w': res = motion_word_forward(count, false); break;
        case 'W': res = motion_word_forward(count, true); break;
        case 'b': res = motion_word_backward(count, false); break;
        case 'B': res = motion_word_backward(count, true); break;
        case 'e': res = motion_word_end(count, false); break;
        case 'E': res = motion_word_end(count, true); break;
        case '%': res = motion_bracket_match(); break;
        case 'G':
            res.y = (count > 0 && E.count1 > 0) ? count - 1 : E.num_lines - 1;
            if (res.y < 0) res.y = 0;
            if (res.y >= E.num_lines) res.y = E.num_lines - 1;
            res.x = 0;
            res.linewise = true;
            res.valid = true;
            break;
        case 'H':
            res.y = min(E.num_lines - 1, E.row_off + (count - 1));
            res.x = 0;
            res.linewise = true;
            res.valid = true;
            break;
        case 'M':
            res.y = min(E.num_lines - 1, E.row_off + E.screen_rows / 2);
            res.x = 0;
            res.linewise = true;
            res.valid = true;
            break;
        case 'L':
            res.y = max(0, min(E.num_lines - 1, E.row_off + E.screen_rows - count));
            res.x = 0;
            res.linewise = true;
            res.valid = true;
            break;
        case '+':
        case '\r':
        case '\n':
            res.y = min(E.num_lines - 1, E.cy + count);
            res.x = 0;
            if (res.y < E.num_lines) {
                while (res.x < E.lines[res.y].len && is_blank(E.lines[res.y].chars[res.x])) res.x++;
            }
            res.linewise = true;
            res.valid = true;
            break;
        case '-':
            res.y = max(0, E.cy - count);
            res.x = 0;
            if (res.y < E.num_lines) {
                while (res.x < E.lines[res.y].len && is_blank(E.lines[res.y].chars[res.x])) res.x++;
            }
            res.linewise = true;
            res.valid = true;
            break;
        case '|':
            res.x = max(0, count - 1);
            if (E.cy < E.num_lines && res.x >= E.lines[E.cy].len) {
                res.x = max(0, E.lines[E.cy].len - 1);
            }
            res.valid = true;
            break;
        case 'f':
        case 'F':
        case 't':
        case 'T':
            if (E.inline_char) res = motion_inline_find((char)key, E.inline_char, count);
            break;
        case '{': {
            int r = res.y;
            for (int n = 0; n < count; n++) {
                while (r > 0 && E.lines[r].len == 0) r--;
                while (r > 0 && E.lines[r].len > 0) r--;
            }
            res.y = r;
            res.x = 0;
            res.linewise = false;
            res.valid = true;
            break;
        }
        case '}': {
            int r = res.y;
            for (int n = 0; n < count; n++) {
                while (r < E.num_lines - 1 && E.lines[r].len == 0) r++;
                while (r < E.num_lines - 1 && E.lines[r].len > 0) r++;
            }
            res.y = r;
            res.x = 0;
            res.linewise = false;
            res.valid = true;
            break;
        }
        case ';':
            if (E.inline_cmd) res = motion_inline_find(E.inline_cmd, E.inline_char, count);
            break;
        case ',':
            if (E.inline_cmd) {
                char rev = E.inline_cmd;
                if (rev == 'f') rev = 'F';
                else if (rev == 'F') rev = 'f';
                else if (rev == 't') rev = 'T';
                else if (rev == 'T') rev = 't';
                res = motion_inline_find(rev, E.inline_char, count);
            }
            break;
    }
    return res;
}

bool evaluate_text_object(char scope, char target, int *out_y1, int *out_x1, int *out_y2, int *out_x2) {
    if (E.num_lines == 0 || E.cy < 0 || E.cy >= E.num_lines) return false;

    /* Paragraph text object ('p') */
    if (target == 'p') {
        int start_y = E.cy;
        int end_y = E.cy;
        bool on_blank = (E.lines[E.cy].len == 0);
        if (!on_blank) {
            while (start_y > 0 && E.lines[start_y - 1].len > 0) start_y--;
            while (end_y + 1 < E.num_lines && E.lines[end_y + 1].len > 0) end_y++;
            if (scope == 'a') {
                while (end_y + 1 < E.num_lines && E.lines[end_y + 1].len == 0) end_y++;
            }
        } else {
            while (start_y > 0 && E.lines[start_y - 1].len == 0) start_y--;
            while (end_y + 1 < E.num_lines && E.lines[end_y + 1].len == 0) end_y++;
        }
        *out_y1 = start_y;
        *out_x1 = 0;
        *out_y2 = end_y;
        *out_x2 = E.lines[end_y].len;
        return true;
    }

    Line *line = &E.lines[E.cy];
    int len = line->len;
    if (len == 0) return false;

    int cx = min(E.cx, len - 1);
    if (cx < 0) cx = 0;

    /* Normalize target aliases */
    if (target == 'b' || target == ')') target = '(';
    if (target == 'B' || target == '}') target = '{';
    if (target == ']') target = '[';
    if (target == '>') target = '<';

    /* 1. Words ('w' and 'W') */
    if (target == 'w' || target == 'W') {
        bool big = (target == 'W');
        char c = line->chars[cx];
        bool on_word = big ? !is_blank(c) : is_word_char(c);
        bool on_blank = is_blank(c);

        int start = cx;
        int end = cx;

        if (on_word) {
            while (start > 0) {
                char prev = line->chars[start - 1];
                if (big ? !is_blank(prev) : is_word_char(prev)) start--;
                else break;
            }
            while (end + 1 < len) {
                char next = line->chars[end + 1];
                if (big ? !is_blank(next) : is_word_char(next)) end++;
                else break;
            }
        } else if (on_blank) {
            while (start > 0 && is_blank(line->chars[start - 1])) start--;
            while (end + 1 < len && is_blank(line->chars[end + 1])) end++;
        } else {
            /* Punctuation */
            while (start > 0 && !is_word_char(line->chars[start - 1]) && !is_blank(line->chars[start - 1])) start--;
            while (end + 1 < len && !is_word_char(line->chars[end + 1]) && !is_blank(line->chars[end + 1])) end++;
        }

        if (scope == 'a') {
            if (end + 1 < len && is_blank(line->chars[end + 1])) {
                while (end + 1 < len && is_blank(line->chars[end + 1])) end++;
            } else {
                while (start > 0 && is_blank(line->chars[start - 1])) start--;
            }
        }

        *out_y1 = E.cy;
        *out_x1 = start;
        *out_y2 = E.cy;
        *out_x2 = end;
        return true;
    }

    /* 2. Quotes: '"', '\'', '`' */
    if (target == '"' || target == '\'' || target == '`') {
        int first_q = -1, second_q = -1;
        for (int i = 0; i < len; i++) {
            if (line->chars[i] == target) {
                if (first_q == -1) first_q = i;
                else {
                    second_q = i;
                    if (first_q <= cx && cx <= second_q) break;
                    if (i > cx) break;
                    first_q = -1;
                    second_q = -1;
                }
            }
        }
        if (first_q == -1 || second_q == -1) return false;

        *out_y1 = E.cy;
        *out_y2 = E.cy;
        if (scope == 'i') {
            *out_x1 = first_q + 1;
            *out_x2 = max(first_q + 1, second_q - 1);
        } else {
            *out_x1 = first_q;
            *out_x2 = second_q;
        }
        return true;
    }

    /* 3. Brackets: '(', '[', '{', '<' */
    if (target == '(' || target == '[' || target == '{' || target == '<') {
        char open_ch = target;
        char close_ch = (target == '(') ? ')' :
                        (target == '[') ? ']' :
                        (target == '{') ? '}' : '>';

        int depth = 0;
        int open_y = -1, open_x = -1;
        int y = E.cy, x = cx;

        while (y >= 0) {
            Line *l = &E.lines[y];
            while (x >= 0) {
                if (l->chars[x] == close_ch && !(y == E.cy && x == cx)) depth++;
                else if (l->chars[x] == open_ch) {
                    if (depth == 0) {
                        open_y = y;
                        open_x = x;
                        goto found_open;
                    }
                    depth--;
                }
                x--;
            }
            y--;
            if (y >= 0) x = E.lines[y].len - 1;
        }
found_open:
        if (open_y == -1) return false;

        depth = 1;
        y = open_y;
        x = open_x + 1;
        int close_y = -1, close_x = -1;

        while (y < E.num_lines) {
            Line *l = &E.lines[y];
            while (x < l->len) {
                if (l->chars[x] == open_ch) depth++;
                else if (l->chars[x] == close_ch) {
                    depth--;
                    if (depth == 0) {
                        close_y = y;
                        close_x = x;
                        goto found_close;
                    }
                }
                x++;
            }
            y++;
            x = 0;
        }
found_close:
        if (close_y == -1) return false;

        if (scope == 'i') {
            if (open_y == close_y) {
                *out_y1 = open_y;
                *out_y2 = close_y;
                *out_x1 = open_x + 1;
                *out_x2 = max(open_x + 1, close_x - 1);
            } else {
                *out_y1 = open_y;
                *out_x1 = open_x + 1;
                *out_y2 = close_y;
                *out_x2 = max(0, close_x - 1);
            }
        } else {
            *out_y1 = open_y;
            *out_x1 = open_x;
            *out_y2 = close_y;
            *out_x2 = close_x;
        }
        return true;
    }

    return false;
}
