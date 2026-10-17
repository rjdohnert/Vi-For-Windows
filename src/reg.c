#include "vi.h"
#include "reg.h"
#include "buffer.h"
#include "undo.h"

#pragma comment(lib, "user32.lib")

void reg_free(Register *r) {
    if (r->data) {
        free(r->data);
        r->data = NULL;
    }
    r->len = 0;
    r->type = VI_REG_NONE;
}

static void win32_clipboard_set(const char *data, int len) {
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, len + 1);
    if (hGlob) {
        char *pGlob = (char *)GlobalLock(hGlob);
        if (pGlob) {
            memcpy(pGlob, data, len);
            pGlob[len] = '\0';
            GlobalUnlock(hGlob);
            SetClipboardData(CF_TEXT, hGlob);
        }
    }
    CloseClipboard();
}

static char *win32_clipboard_get(int *out_len) {
    if (!OpenClipboard(NULL)) return NULL;
    HANDLE hData = GetClipboardData(CF_TEXT);
    char *text = NULL;
    if (hData) {
        char *psz = (char *)GlobalLock(hData);
        if (psz) {
            text = _strdup(psz);
            if (out_len) *out_len = (int)strlen(text);
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
    return text;
}

void reg_set(char reg_name, const char *data, int len, RegType type) {
    /* Windows System Clipboard Bridge: "+ and "* */
    if (reg_name == '+' || reg_name == '*') {
        win32_clipboard_set(data, len);
    }

    Register *target = NULL;
    bool append = false;

    if (reg_name >= 'a' && reg_name <= 'z') {
        target = &E.reg_named[reg_name - 'a'];
    } else if (reg_name >= 'A' && reg_name <= 'Z') {
        target = &E.reg_named[reg_name - 'A'];
        append = true;
    } else if (reg_name >= '1' && reg_name <= '9') {
        target = &E.reg_numbered[reg_name - '1'];
    } else {
        target = &E.reg_unnamed;
    }

    if (append && target->data) {
        int new_len = target->len + len;
        target->data = (char *)realloc(target->data, new_len + 1);
        memcpy(&target->data[target->len], data, len);
        target->data[new_len] = '\0';
        target->len = new_len;
    } else {
        reg_free(target);
        target->data = (char *)malloc(len + 1);
        memcpy(target->data, data, len);
        target->data[len] = '\0';
        target->len = len;
        target->type = type;
    }

    if (target != &E.reg_unnamed) {
        reg_free(&E.reg_unnamed);
        E.reg_unnamed.data = _strdup(target->data);
        E.reg_unnamed.len = target->len;
        E.reg_unnamed.type = target->type;
    }
}

void push_numbered_reg(const char *data, int len) {
    reg_free(&E.reg_numbered[8]);
    for (int i = 8; i > 0; i--) {
        E.reg_numbered[i] = E.reg_numbered[i - 1];
    }
    memset(&E.reg_numbered[0], 0, sizeof(Register));
    E.reg_numbered[0].data = (char *)malloc(len + 1);
    memcpy(E.reg_numbered[0].data, data, len);
    E.reg_numbered[0].data[len] = '\0';
    E.reg_numbered[0].len = len;
    E.reg_numbered[0].type = VI_REG_LINE;
}

Register *reg_get(char reg_name) {
    /* Windows System Clipboard Bridge: "+ and "* */
    if (reg_name == '+' || reg_name == '*') {
        int len = 0;
        char *clip_data = win32_clipboard_get(&len);
        if (clip_data) {
            reg_free(&E.reg_clipboard);
            E.reg_clipboard.data = clip_data;
            E.reg_clipboard.len = len;
            /* If contains newline, treat as linewise, else characterwise */
            E.reg_clipboard.type = (strchr(clip_data, '\n') != NULL) ? VI_REG_LINE : VI_REG_CHAR;
            return &E.reg_clipboard;
        }
    }

    if (reg_name >= 'a' && reg_name <= 'z') return &E.reg_named[reg_name - 'a'];
    if (reg_name >= 'A' && reg_name <= 'Z') return &E.reg_named[reg_name - 'A'];
    if (reg_name >= '1' && reg_name <= '9') return &E.reg_numbered[reg_name - '1'];
    return &E.reg_unnamed;
}

void do_paste(bool after, int count) {
    Register *r = reg_get(E.active_reg);
    if (!r || !r->data || r->len == 0) return;
    if (count <= 0) count = 1;

    save_undo();

    for (int n = 0; n < count; n++) {
        if (r->type == VI_REG_LINE) {
            int insert_at = (E.num_lines == 0) ? 0 : (after ? E.cy + 1 : E.cy);
            const char *p = r->data;
            while (*p) {
                const char *nl = strchr(p, '\n');
                int len = nl ? (int)(nl - p) : (int)strlen(p);
                /* Strip trailing CR if pasted from Windows CRLF clipboard */
                if (len > 0 && p[len - 1] == '\r') len--;
                insert_row(insert_at++, p, len);
                if (!nl) break;
                p = nl + 1;
            }
            E.cy = (E.num_lines == 0) ? 0 : (after ? E.cy + 1 : E.cy);
            if (E.cy >= E.num_lines) E.cy = E.num_lines - 1;
            if (E.cy < 0) E.cy = 0;
            E.cx = 0;
            if (E.num_lines > 0) {
                while (E.cx < E.lines[E.cy].len && isspace((unsigned char)E.lines[E.cy].chars[E.cx])) E.cx++;
            }
        } else if (r->type == VI_REG_BLOCK) {
            int insert_x = after ? (E.lines[E.cy].len > 0 ? E.cx + 1 : 0) : E.cx;
            const char *p = r->data;
            int r_idx = 0;
            while (*p) {
                const char *nl = strchr(p, '\n');
                int len = nl ? (int)(nl - p) : (int)strlen(p);
                int target_y = E.cy + r_idx;
                if (target_y >= E.num_lines) {
                    insert_row(target_y, "", 0);
                }
                Line *line = &E.lines[target_y];
                while (line->len < insert_x) {
                    line_insert_char(line, line->len, ' ');
                }
                line_insert_str(line, insert_x, p, len);
                r_idx++;
                if (!nl) break;
                p = nl + 1;
            }
        } else {
            if (E.num_lines == 0) {
                insert_row(0, "", 0);
                E.cy = 0;
                E.cx = 0;
            }
            if (E.cy < 0) E.cy = 0;
            if (E.cy >= E.num_lines) E.cy = E.num_lines - 1;
            if (E.cx < 0) E.cx = 0;
            if (E.cx > E.lines[E.cy].len) E.cx = E.lines[E.cy].len;

            int pos = after ? (E.lines[E.cy].len > 0 ? E.cx + 1 : 0) : E.cx;
            if (strchr(r->data, '\n') == NULL) {
                line_insert_str(&E.lines[E.cy], pos, r->data, r->len);
                E.cx = pos + r->len - 1;
                if (E.cx < 0) E.cx = 0;
            } else {
                int suffix_len = E.lines[E.cy].len - pos;
                char *suffix = _strdup(&E.lines[E.cy].chars[pos]);
                E.lines[E.cy].len = pos;
                E.lines[E.cy].chars[pos] = '\0';

                const char *p = r->data;
                int cur_row = E.cy;
                bool first_seg = true;
                while (true) {
                    const char *nl = strchr(p, '\n');
                    int seg_len = nl ? (int)(nl - p) : (int)strlen(p);
                    if (seg_len > 0 && p[seg_len - 1] == '\r') seg_len--;
                    if (first_seg) {
                        line_insert_str(&E.lines[cur_row], pos, p, seg_len);
                        first_seg = false;
                    } else {
                        cur_row++;
                        insert_row(cur_row, p, seg_len);
                    }
                    if (!nl) break;
                    p = nl + 1;
                }
                int end_x = E.lines[cur_row].len;
                if (suffix_len > 0) {
                    line_insert_str(&E.lines[cur_row], end_x, suffix, suffix_len);
                }
                free(suffix);
                E.cy = cur_row;
                E.cx = max(0, end_x - 1);
            }
        }
    }

    E.active_reg = 0;
}
