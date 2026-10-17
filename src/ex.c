#include "vi.h"
#include "ex.h"
#include "buffer.h"
#include "undo.h"
#include "search.h"
#include "ops.h"
#include "screen.h"
#include "term.h"
#include "window.h"
#include "tags.h"
#include "explorer.h"
#include "syntax.h"
#include "diff.h"
#include "quickfix.h"
#include "session.h"
#include "tab.h"
#include "fold.h"

static void execute_subshell(void) {
    term_disable_raw();
    printf("\x1b[2J\x1b[H");
    fflush(stdout);

    const char *shell = getenv("COMSPEC");
    if (!shell || *shell == '\0') shell = "cmd.exe";

    printf("====================================================================\r\n");
    printf("  Vi for Windows Interactive Subshell (type 'exit' to return to Vi)\r\n");
    printf("====================================================================\r\n\r\n");
    fflush(stdout);

    system(shell);

    term_enable_raw();
    printf("\x1b[2J\x1b[H");
    editor_refresh_screen();
}

bool parse_ex_address(const char **s, int *addr) {
    while (**s == ' ') (*s)++;
    if (**s == '\0') return false;

    if (**s == '.') {
        *addr = E.cy + 1;
        (*s)++;
    } else if (**s == '$') {
        *addr = E.num_lines;
        (*s)++;
    } else if (**s == '\'') {
        (*s)++;
        char m = **s;
        if (m >= 'a' && m <= 'z' && E.marks[m - 'a'].set) {
            *addr = E.marks[m - 'a'].y + 1;
            (*s)++;
        } else if (m == '<') {
            *addr = min(E.visual_anchor_y, E.cy) + 1;
            (*s)++;
        } else if (m == '>') {
            *addr = max(E.visual_anchor_y, E.cy) + 1;
            (*s)++;
        } else return false;
    } else if (isdigit((unsigned char)**s)) {
        *addr = (int)strtol(*s, (char **)s, 10);
    } else {
        return false;
    }

    while (**s == '+' || **s == '-') {
        char op = **s;
        (*s)++;
        int off = 1;
        if (isdigit((unsigned char)**s)) {
            off = (int)strtol(*s, (char **)s, 10);
        }
        if (op == '+') *addr += off;
        else *addr -= off;
    }

    if (*addr < 0) *addr = 0;
    if (*addr > E.num_lines) *addr = E.num_lines;
    return true;
}

void parse_ex_range(const char **s, int *start, int *end, bool *has_range) {
    while (**s == ' ') (*s)++;
    *start = E.cy + 1;
    *end = E.cy + 1;
    *has_range = false;

    if (**s == '%') {
        *start = 1;
        *end = E.num_lines;
        *has_range = true;
        (*s)++;
        return;
    }

    if (parse_ex_address(s, start)) {
        *has_range = true;
        if (*start < 1) *start = 1;
        *end = *start;
        while (**s == ' ') (*s)++;
        if (**s == ',' || **s == ';') {
            (*s)++;
            parse_ex_address(s, end);
            if (*end < 1) *end = 1;
        }
    }
}

void execute_substitute(int start_row, int end_row, const char *args) {
    if (*args == '\0') return;
    char delim = *args++;
    if (delim == '\0') return;

    char find_pat[256] = {0};
    int i = 0;
    while (*args && *args != delim && i < (int)sizeof(find_pat) - 1) {
        if (*args == '\\' && *(args + 1) == delim) {
            args++;
            find_pat[i++] = *args++;
        } else if (*args == '\\' && *(args + 1)) {
            find_pat[i++] = *args++;
            if (i < (int)sizeof(find_pat) - 1) {
                find_pat[i++] = *args++;
            }
        } else {
            find_pat[i++] = *args++;
        }
    }
    find_pat[i] = '\0';
    if (*args == delim) args++;

    char repl_str[256] = {0};
    i = 0;
    while (*args && *args != delim && i < (int)sizeof(repl_str) - 1) {
        if (*args == '\\' && *(args + 1) == delim) {
            args++;
            repl_str[i++] = *args++;
        } else if (*args == '\\' && *(args + 1)) {
            repl_str[i++] = *args++;
            if (i < (int)sizeof(repl_str) - 1) {
                repl_str[i++] = *args++;
            }
        } else {
            repl_str[i++] = *args++;
        }
    }
    repl_str[i] = '\0';
    if (*args == delim) args++;

    bool global = false;
    bool ic = E.opt.ignorecase;
    while (*args) {
        if (*args == 'g') global = true;
        if (*args == 'i') ic = true;
        args++;
    }

    if (strlen(find_pat) == 0) return;

    save_undo();
    int total_sub = 0;
    int lines_affected = 0;

    for (int r = start_row - 1; r < end_row && r < E.num_lines; r++) {
        Line *line = &E.lines[r];
        int match_len = 0;
        const char *match = regex_search_line(find_pat, line->chars, ic, &match_len);
        if (!match) continue;

        lines_affected++;
        int cap = line->cap + (int)strlen(repl_str) * 2 + 16;
        char *new_chars = (char *)malloc(cap);
        int new_len = 0;
        const char *src = line->chars;

        while (match) {
            total_sub++;
            int prefix = (int)(match - src);
            while (new_len + prefix + (int)strlen(repl_str) + 16 > cap) {
                cap *= 2;
                new_chars = (char *)realloc(new_chars, cap);
            }
            memcpy(&new_chars[new_len], src, prefix);
            new_len += prefix;

            int rlen = (int)strlen(repl_str);
            memcpy(&new_chars[new_len], repl_str, rlen);
            new_len += rlen;

            /* Advance past the match */
            if (match_len > 0) {
                src = match + match_len;
            } else {
                /* Zero-length match (e.g. ^ or $): copy character to avoid infinite loop */
                if (*match != '\0') {
                    while (new_len + 2 > cap) {
                        cap *= 2;
                        new_chars = (char *)realloc(new_chars, cap);
                    }
                    new_chars[new_len++] = *match;
                    src = match + 1;
                } else {
                    src = match;
                    break;
                }
            }

            if (!global || find_pat[0] == '^') break;
            if (*src == '\0') break;
            match = regex_search_line(find_pat, src, ic, &match_len);
        }

        int remainder = (int)strlen(src);
        while (new_len + remainder + 1 > cap) {
            cap *= 2;
            new_chars = (char *)realloc(new_chars, cap);
        }
        memcpy(&new_chars[new_len], src, remainder);
        new_len += remainder;
        new_chars[new_len] = '\0';

        free(line->chars);
        line->chars = new_chars;
        line->len = new_len;
        line->cap = cap;
        E.dirty = true;
    }

    set_status("%d substitutions on %d lines", total_sub, lines_affected);
}

static void execute_global(int start_row, int end_row, bool invert, const char *args) {
    while (*args == ' ') args++;
    if (*args == '\0') return;
    char delim = *args++;
    if (delim == '\0') return;

    char pat[256] = {0};
    int i = 0;
    while (*args && *args != delim && i < (int)sizeof(pat) - 1) {
        if (*args == '\\' && *(args + 1)) args++;
        pat[i++] = *args++;
    }
    pat[i] = '\0';
    if (*args == delim) args++;
    while (*args == ' ') args++;

    const char *subcmd = args;
    if (*subcmd == '\0') subcmd = "p";

    if (E.num_lines == 0 || strlen(pat) == 0) return;

    bool *matches = (bool *)calloc(E.num_lines, sizeof(bool));
    int match_count = 0;
    for (int r = start_row - 1; r < end_row && r < E.num_lines; r++) {
        bool m = (regex_search_line(pat, E.lines[r].chars, E.opt.ignorecase, NULL) != NULL);
        if (invert) m = !m;
        if (m) {
            matches[r] = true;
            match_count++;
        }
    }

    if (match_count == 0) {
        free(matches);
        set_status("Pattern not found: %s", pat);
        return;
    }

    save_undo();

    if (strcmp(subcmd, "d") == 0) {
        int deleted = 0;
        for (int r = E.num_lines - 1; r >= 0; r--) {
            if (matches[r]) {
                delete_row(r);
                deleted++;
            }
        }
        if (E.num_lines == 0) insert_row(0, "", 0);
        E.cy = min(E.cy, E.num_lines - 1);
        E.cx = 0;
        set_status("%d fewer lines", deleted);
    } else if (*subcmd == 's') {
        for (int r = 0; r < E.num_lines; r++) {
            if (matches[r]) {
                execute_substitute(r + 1, r + 1, subcmd + 1);
            }
        }
    } else if (strcmp(subcmd, "p") == 0) {
        for (int r = 0; r < E.num_lines; r++) {
            if (matches[r]) E.cy = r;
        }
        E.cx = 0;
        set_status("%d matching lines", match_count);
    } else {
        for (int r = 0; r < E.num_lines; r++) {
            if (matches[r]) {
                E.cy = r;
                execute_ex_command(subcmd);
            }
        }
    }

    free(matches);
}

static void execute_move_or_copy(int start, int end, const char *args, bool is_move) {
    while (*args == ' ') args++;
    int dest = 0;
    if (!parse_ex_address(&args, &dest)) {
        set_status("Invalid destination address");
        return;
    }

    if (start < 1) start = 1;
    if (end > E.num_lines) end = E.num_lines;
    if (start > end) return;

    if (is_move && dest >= start && dest <= end) {
        set_status("Cannot move range into itself");
        return;
    }

    save_undo();

    int count = end - start + 1;
    char **copied_lines = (char **)malloc(sizeof(char *) * count);
    for (int i = 0; i < count; i++) {
        copied_lines[i] = _strdup(E.lines[start - 1 + i].chars);
    }

    if (is_move) {
        for (int i = 0; i < count; i++) {
            delete_row(start - 1);
        }
        if (dest > end) dest -= count;
    }

    int insert_pos = dest;
    for (int i = 0; i < count; i++) {
        insert_row(insert_pos++, copied_lines[i], (int)strlen(copied_lines[i]));
        free(copied_lines[i]);
    }
    free(copied_lines);

    E.cy = min(E.num_lines - 1, insert_pos - 1);
    E.cx = 0;
    set_status("%d lines %s", count, is_move ? "moved" : "copied");
}

static void execute_filter_command(int start, int end, const char *cmd) {
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    if (start < 1) start = 1;
    if (end > E.num_lines) end = E.num_lines;
    if (start > end) return;

    save_undo();

    char temp_in[MAX_PATH], temp_out[MAX_PATH];
    char temp_dir[MAX_PATH];
    GetTempPathA(sizeof(temp_dir), temp_dir);
    GetTempFileNameA(temp_dir, "vi_in", 0, temp_in);
    GetTempFileNameA(temp_dir, "vi_out", 0, temp_out);

    FILE *fin = fopen(temp_in, "wb");
    if (!fin) return;
    for (int r = start - 1; r < end; r++) {
        fputs(E.lines[r].chars, fin);
        fputc('\n', fin);
    }
    fclose(fin);

    char full_cmd[1024];
    snprintf(full_cmd, sizeof(full_cmd), "cmd.exe /c %s < \"%s\" > \"%s\"", cmd, temp_in, temp_out);

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (CreateProcessA(NULL, full_cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 5000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    FILE *fout = fopen(temp_out, "rb");
    if (fout) {
        int count_to_del = end - start + 1;
        for (int i = 0; i < count_to_del; i++) {
            delete_row(start - 1);
        }

        char line_buf[4096];
        int insert_pos = start - 1;
        int inserted = 0;
        while (fgets(line_buf, sizeof(line_buf), fout)) {
            char *nl = strchr(line_buf, '\n');
            if (nl) *nl = '\0';
            char *cr = strchr(line_buf, '\r');
            if (cr) *cr = '\0';
            insert_row(insert_pos++, line_buf, (int)strlen(line_buf));
            inserted++;
        }
        fclose(fout);
        if (E.num_lines == 0) insert_row(0, "", 0);
        E.cy = min(start - 1, E.num_lines - 1);
        E.cx = 0;
        set_status("%d lines filtered through '%s'", inserted, cmd);
    }

    remove(temp_in);
    remove(temp_out);
}

void execute_set_command(const char *args) {
    while (*args == ' ') args++;
    if (*args == '\0') {
        set_status("%snu  %sro  %sic  %ssm  %sruler  %sai  %shls  %ssyn  %scp  %sis  %swrap  %smouse  ff=%s  colo=%s  ts=%d  sw=%d",
                   E.opt.number ? "" : "no",
                   E.readonly ? "" : "no",
                   E.opt.ignorecase ? "" : "no",
                   E.opt.showmode ? "" : "no",
                   E.opt.ruler ? "" : "no",
                   E.opt.autoindent ? "" : "no",
                   E.opt.hlsearch ? "" : "no",
                   E.opt.syntax ? "" : "no",
                   E.opt.compatible ? "" : "no",
                   E.opt.incsearch ? "" : "no",
                   E.opt.wrap ? "" : "no",
                   E.opt.mouse ? "" : "no",
                   E.is_crlf ? "dos" : "unix",
                   syntax_get_colorscheme(),
                   E.opt.tabstop, E.opt.shiftwidth);
        return;
    }

    if (strcmp(args, "number") == 0 || strcmp(args, "nu") == 0) E.opt.number = true;
    else if (strcmp(args, "nonumber") == 0 || strcmp(args, "nonu") == 0) E.opt.number = false;
    else if (strcmp(args, "readonly") == 0 || strcmp(args, "ro") == 0) E.readonly = true;
    else if (strcmp(args, "noreadonly") == 0 || strcmp(args, "noro") == 0) E.readonly = false;
    else if (strcmp(args, "ignorecase") == 0 || strcmp(args, "ic") == 0) E.opt.ignorecase = true;
    else if (strcmp(args, "noignorecase") == 0 || strcmp(args, "noic") == 0) E.opt.ignorecase = false;
    else if (strcmp(args, "showmode") == 0 || strcmp(args, "sm") == 0) E.opt.showmode = true;
    else if (strcmp(args, "noshowmode") == 0 || strcmp(args, "nosm") == 0) E.opt.showmode = false;
    else if (strcmp(args, "ruler") == 0) E.opt.ruler = true;
    else if (strcmp(args, "noruler") == 0) E.opt.ruler = false;
    else if (strcmp(args, "autoindent") == 0 || strcmp(args, "ai") == 0) E.opt.autoindent = true;
    else if (strcmp(args, "noautoindent") == 0 || strcmp(args, "noai") == 0) E.opt.autoindent = false;
    else if (strcmp(args, "hlsearch") == 0 || strcmp(args, "hls") == 0) { E.opt.hlsearch = true; E.opt.nohl_active = false; }
    else if (strcmp(args, "nohlsearch") == 0 || strcmp(args, "nohls") == 0) { E.opt.hlsearch = false; }
    else if (strcmp(args, "incsearch") == 0 || strcmp(args, "is") == 0) { E.opt.incsearch = true; }
    else if (strcmp(args, "noincsearch") == 0 || strcmp(args, "nois") == 0) { E.opt.incsearch = false; }
    else if (strcmp(args, "wrap") == 0) { E.opt.wrap = true; }
    else if (strcmp(args, "nowrap") == 0) { E.opt.wrap = false; }
    else if (strcmp(args, "mouse=a") == 0 || strcmp(args, "mouse") == 0) { term_set_mouse(true); }
    else if (strcmp(args, "nomouse") == 0 || strcmp(args, "mouse=") == 0) { term_set_mouse(false); }
    else if (strcmp(args, "mouse?") == 0) { set_status(E.opt.mouse ? "mouse=a" : "mouse="); }
    else if (strcmp(args, "autoread") == 0 || strcmp(args, "ar") == 0) { E.opt.autoread = true; }
    else if (strcmp(args, "noautoread") == 0 || strcmp(args, "noar") == 0) { E.opt.autoread = false; }
    else if (strcmp(args, "undofile") == 0 || strcmp(args, "udf") == 0) { E.opt.undofile = true; }
    else if (strcmp(args, "noundofile") == 0 || strcmp(args, "noudf") == 0) { E.opt.undofile = false; }
    else if (strcmp(args, "list") == 0) { E.opt.list = true; }
    else if (strcmp(args, "nolist") == 0) { E.opt.list = false; }
    else if (strncmp(args, "fdm=", 4) == 0 || strncmp(args, "foldmethod=", 11) == 0) {
        const char *eq = strchr(args, '=');
        if (eq) {
            strncpy(E.opt.foldmethod, eq + 1, sizeof(E.opt.foldmethod) - 1);
            E.opt.foldmethod[sizeof(E.opt.foldmethod) - 1] = '\0';
        }
    }
    else if (strcmp(args, "syntax=on") == 0 || strcmp(args, "syn=on") == 0 || strcmp(args, "syntax") == 0 || strcmp(args, "syn") == 0) E.opt.syntax = true;
    else if (strcmp(args, "syntax=off") == 0 || strcmp(args, "syn=off") == 0 || strcmp(args, "nosyntax") == 0 || strcmp(args, "nosyn") == 0) E.opt.syntax = false;
    else if (strcmp(args, "compatible") == 0 || strcmp(args, "cp") == 0) E.opt.compatible = true;
    else if (strcmp(args, "nocompatible") == 0 || strcmp(args, "nocp") == 0) E.opt.compatible = false;
    else if (strcmp(args, "ff=dos") == 0 || strcmp(args, "fileformat=dos") == 0) { E.is_crlf = true; E.dirty = true; }
    else if (strcmp(args, "ff=unix") == 0 || strcmp(args, "fileformat=unix") == 0) { E.is_crlf = false; E.dirty = true; }
    else if (strncmp(args, "colo=", 5) == 0 || strncmp(args, "colorscheme=", 12) == 0) {
        const char *eq = strchr(args, '=');
        if (eq) syntax_set_colorscheme(eq + 1);
    } else if (strncmp(args, "colo ", 5) == 0 || strncmp(args, "colorscheme ", 12) == 0) {
        const char *val = (strncmp(args, "colorscheme ", 12) == 0) ? args + 12 : args + 5;
        while (*val == ' ') val++;
        syntax_set_colorscheme(val);
    } else if (strncmp(args, "ts=", 3) == 0 || strncmp(args, "tabstop=", 8) == 0) {
        const char *eq = strchr(args, '=');
        if (eq) E.opt.tabstop = max(1, atoi(eq + 1));
    } else if (strncmp(args, "sw=", 3) == 0 || strncmp(args, "shiftwidth=", 11) == 0) {
        const char *eq = strchr(args, '=');
        if (eq) E.opt.shiftwidth = max(1, atoi(eq + 1));
    } else {
        set_status("Unknown option: %s", args);
    }
}

void execute_ex_command(const char *cmd) {
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    if (*cmd == '!') {
        char expanded[1024] = {0};
        const char *src = cmd + 1;
        int dst = 0;
        while (*src && dst < (int)sizeof(expanded) - 2) {
            if (*src == '%' && E.filename) {
                int flen = (int)strlen(E.filename);
                if (dst + flen + 2 < (int)sizeof(expanded) - 1) {
                    expanded[dst++] = '"';
                    memcpy(&expanded[dst], E.filename, flen);
                    dst += flen;
                    expanded[dst++] = '"';
                }
                src++;
            } else {
                expanded[dst++] = *src++;
            }
        }
        expanded[dst] = '\0';

        term_disable_raw();
        printf("\r\n");
        system(expanded);
        printf("\r\n[Hit return to continue]");
        fflush(stdout);
        while (getchar() != '\n');
        term_enable_raw();
        return;
    }

    int start = 1, end = E.num_lines;
    bool has_range = false;
    parse_ex_range(&cmd, &start, &end, &has_range);
    while (*cmd == ' ') cmd++;

    if (*cmd == '\0') {
        if (has_range) {
            E.cy = end - 1;
            E.cx = 0;
        }
        return;
    }

    if (*cmd == '!' && has_range) {
        execute_filter_command(start, end, cmd + 1);
        return;
    }

    if (strcmp(cmd, "Explore") == 0 || strcmp(cmd, "E") == 0 || strcmp(cmd, "Sex") == 0) {
        if (strcmp(cmd, "Sex") == 0) window_split(false);
        explorer_open(".");
        return;
    }

    if (strcmp(cmd, "q") == 0) {
        if (E.dirty) {
            set_status("E37: No write since last change (add ! to override)");
        } else {
            exit(0);
        }
    } else if (strcmp(cmd, "q!") == 0) {
        exit(0);
    } else if (strcmp(cmd, "w") == 0) {
        editor_save(NULL, false);
    } else if (strcmp(cmd, "w!") == 0) {
        editor_save(NULL, true);
    } else if (strncmp(cmd, "w! ", 3) == 0) {
        editor_save(cmd + 3, true);
    } else if (strncmp(cmd, "w ", 2) == 0) {
        editor_save(cmd + 2, false);
    } else if (strcmp(cmd, "wq") == 0 || strcmp(cmd, "x") == 0) {
        if (editor_save(NULL, false)) exit(0);
    } else if (strcmp(cmd, "wq!") == 0 || strcmp(cmd, "x!") == 0) {
        if (editor_save(NULL, true)) exit(0);
    } else if (strcmp(cmd, "e!") == 0 || strcmp(cmd, "edit!") == 0) {
        if (E.filename && E.cur_buf_idx >= 0 && E.cur_buf_idx < E.num_buffers) {
            if (buffer_reload(&E.buffers[E.cur_buf_idx])) {
                clear_undo();
                backup_u_line();
                set_status("\"%s\" %dL reloaded", E.filename, E.num_lines);
            } else {
                set_status("E484: Can't open file %s", E.filename);
            }
        } else {
            set_status("E32: No file name");
        }
    } else if (strncmp(cmd, "e ", 2) == 0 || strncmp(cmd, "edit ", 5) == 0) {
        if (E.dirty) set_status("E37: No write since last change (add ! to override)");
        else {
            const char *path = (strncmp(cmd, "edit ", 5) == 0) ? cmd + 5 : cmd + 2;
            while (*path == ' ') path++;
            if (is_directory(path)) {
                explorer_open(path);
            } else {
                editor_open(path);
            }
        }
    } else if (strncmp(cmd, "e! ", 3) == 0 || strncmp(cmd, "edit! ", 6) == 0) {
        const char *path = (strncmp(cmd, "edit! ", 6) == 0) ? cmd + 6 : cmd + 3;
        while (*path == ' ') path++;
        if (is_directory(path)) {
            explorer_open(path);
        } else {
            editor_open(path);
        }
    } else if (strcmp(cmd, "split") == 0 || strcmp(cmd, "sp") == 0) {
        window_split(false);
    } else if (strcmp(cmd, "vsplit") == 0 || strcmp(cmd, "vs") == 0) {
        window_split(true);
    } else if (strcmp(cmd, "close") == 0 || strcmp(cmd, "clo") == 0) {
        window_close_current();
    } else if (strcmp(cmd, "only") == 0 || strcmp(cmd, "on") == 0) {
        while (E.num_windows > 1) window_close_current();
    } else if (strncmp(cmd, "tag ", 4) == 0) {
        tags_jump(cmd + 4);
    } else if (strncmp(cmd, "ta ", 3) == 0) {
        tags_jump(cmd + 3);
    } else if (strcmp(cmd, "pop") == 0 || strcmp(cmd, "po") == 0) {
        tags_pop();
    } else if (strcmp(cmd, "tags") == 0) {
        tags_list();
    } else if (strncmp(cmd, "colorscheme ", 12) == 0) {
        syntax_set_colorscheme(cmd + 12);
    } else if (strncmp(cmd, "colo ", 5) == 0) {
        syntax_set_colorscheme(cmd + 5);
    } else if (strcmp(cmd, "colorscheme") == 0 || strcmp(cmd, "colo") == 0) {
        set_status("%s", syntax_get_colorscheme());
    } else if (strcmp(cmd, "checktime") == 0) {
        buffer_check_all_modified();
    } else if (strncmp(cmd, "diffsplit ", 10) == 0) {
        diff_split(cmd + 10);
    } else if (strcmp(cmd, "diffthis") == 0) {
        diff_this();
    } else if (strcmp(cmd, "diffoff") == 0) {
        diff_off();
    } else if (strcmp(cmd, "diffupdate") == 0 || strcmp(cmd, "diffup") == 0) {
        diff_compute();
        set_status("Diff updated");
    } else if (strncmp(cmd, "grep ", 5) == 0) {
        quickfix_run_grep(cmd + 5);
    } else if (strncmp(cmd, "copen", 5) == 0) {
        const char *arg = cmd + 5;
        while (*arg == ' ') arg++;
        int h = isdigit((unsigned char)*arg) ? atoi(arg) : 0;
        quickfix_open_window(h);
    } else if (strcmp(cmd, "cclose") == 0 || strcmp(cmd, "ccl") == 0) {
        quickfix_close_window();
    } else if (strcmp(cmd, "cnext") == 0 || strcmp(cmd, "cn") == 0) {
        quickfix_next();
    } else if (strcmp(cmd, "cprev") == 0 || strcmp(cmd, "cp") == 0) {
        quickfix_prev();
    } else if (strcmp(cmd, "clist") == 0 || strcmp(cmd, "cl") == 0) {
        quickfix_list();
    } else if (strncmp(cmd, "find ", 5) == 0 || strncmp(cmd, "fin ", 4) == 0) {
        const char *q = (strncmp(cmd, "find ", 5) == 0) ? cmd + 5 : cmd + 4;
        while (*q == ' ') q++;
        char match[MAX_PATH];
        if (explorer_find_file(q, match, sizeof(match))) {
            editor_open(match);
        } else {
            set_status("E345: Can't find file \"%s\" in path", q);
        }
    } else if (strncmp(cmd, "mksession", 9) == 0 || strncmp(cmd, "mks", 3) == 0) {
        const char *p = (strncmp(cmd, "mksession", 9) == 0) ? cmd + 9 : cmd + 3;
        while (*p == ' ') p++;
        session_save(p);
    } else if (strncmp(cmd, "source ", 7) == 0 || strncmp(cmd, "so ", 3) == 0) {
        const char *p = (strncmp(cmd, "source ", 7) == 0) ? cmd + 7 : cmd + 3;
        while (*p == ' ') p++;
        session_load(p);
    } else if (strncmp(cmd, "tabnew", 6) == 0 || strncmp(cmd, "tabedit", 7) == 0 || strncmp(cmd, "tabe", 4) == 0) {
        const char *p = (strncmp(cmd, "tabedit", 7) == 0) ? cmd + 7 :
                        ((strncmp(cmd, "tabnew", 6) == 0) ? cmd + 6 : cmd + 4);
        while (*p == ' ') p++;
        tab_new(*p ? p : NULL);
    } else if (strcmp(cmd, "tabclose") == 0 || strcmp(cmd, "tabc") == 0) {
        tab_close();
    } else if (strcmp(cmd, "tabnext") == 0 || strcmp(cmd, "tabn") == 0) {
        tab_next();
    } else if (strcmp(cmd, "tabprev") == 0 || strcmp(cmd, "tabp") == 0) {
        tab_prev();
    } else if (strcmp(cmd, "tabonly") == 0 || strcmp(cmd, "tabo") == 0) {
        tab_only();
    } else if (strcmp(cmd, "sh") == 0 || strcmp(cmd, "shell") == 0 ||
               strcmp(cmd, "terminal") == 0 || strcmp(cmd, "term") == 0) {
        execute_subshell();
    } else if (strcmp(cmd, "foldclose") == 0 || strcmp(cmd, "foldc") == 0) {
        fold_close_at(E.cy);
    } else if (strcmp(cmd, "foldopen") == 0 || strcmp(cmd, "foldo") == 0) {
        fold_open_at(E.cy);
    } else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "buffers") == 0) {
        buffer_list();
    } else if (strncmp(cmd, "badd ", 5) == 0) {
        buffer_add(cmd + 5);
    } else if (strcmp(cmd, "bn") == 0 || strcmp(cmd, "bnext") == 0) {
        buffer_next();
    } else if (strcmp(cmd, "bp") == 0 || strcmp(cmd, "bprev") == 0 || strcmp(cmd, "bprevious") == 0) {
        buffer_prev();
    } else if (strncmp(cmd, "bd!", 3) == 0 || strncmp(cmd, "bdelete!", 8) == 0) {
        const char *arg = (strncmp(cmd, "bd!", 3) == 0) ? cmd + 3 : cmd + 8;
        while (*arg == ' ') arg++;
        if (*arg == '\0') {
            buffer_delete(E.cur_buf_idx, true);
        } else if (isdigit((unsigned char)*arg)) {
            int target = atoi(arg);
            int idx = -1;
            for (int i = 0; i < E.num_buffers; i++) {
                if (E.buffers[i].id == target) { idx = i; break; }
            }
            if (idx == -1 && target >= 1 && target <= E.num_buffers) idx = target - 1;
            if (idx >= 0) buffer_delete(idx, true);
            else set_status("E86: Buffer %d does not exist", target);
        } else {
            int idx = -1;
            for (int i = 0; i < E.num_buffers; i++) {
                if (E.buffers[i].filename && strstr(E.buffers[i].filename, arg)) { idx = i; break; }
            }
            if (idx >= 0) buffer_delete(idx, true);
            else set_status("E94: No matching buffer for %s", arg);
        }
    } else if (strncmp(cmd, "bd", 2) == 0 || strncmp(cmd, "bdelete", 7) == 0) {
        const char *arg = (strncmp(cmd, "bd", 2) == 0) ? cmd + 2 : cmd + 7;
        while (*arg == ' ') arg++;
        if (*arg == '\0') {
            buffer_delete(E.cur_buf_idx, false);
        } else if (isdigit((unsigned char)*arg)) {
            int target = atoi(arg);
            int idx = -1;
            for (int i = 0; i < E.num_buffers; i++) {
                if (E.buffers[i].id == target) { idx = i; break; }
            }
            if (idx == -1 && target >= 1 && target <= E.num_buffers) idx = target - 1;
            if (idx >= 0) buffer_delete(idx, false);
            else set_status("E86: Buffer %d does not exist", target);
        } else {
            int idx = -1;
            for (int i = 0; i < E.num_buffers; i++) {
                if (E.buffers[i].filename && strstr(E.buffers[i].filename, arg)) { idx = i; break; }
            }
            if (idx >= 0) buffer_delete(idx, false);
            else set_status("E94: No matching buffer for %s", arg);
        }
    } else if (strcmp(cmd, "b") == 0 || strcmp(cmd, "buffer") == 0) {
        buffer_list();
    } else if (strncmp(cmd, "b ", 2) == 0) {
        buffer_switch_by_name(cmd + 2);
    } else if (strncmp(cmd, "buffer ", 7) == 0) {
        buffer_switch_by_name(cmd + 7);
    } else if (strcmp(cmd, "recover") == 0) {
        swap_recover();
    } else if (strncmp(cmd, "r ", 2) == 0) {
        const char *r_arg = cmd + 2;
        int ins_line = has_range ? end : E.cy + 1;
        if (*r_arg == '!') {
            FILE *pipe = _popen(r_arg + 1, "r");
            if (pipe) {
                save_undo();
                char pbuf[4096];
                int added = 0;
                while (fgets(pbuf, sizeof(pbuf), pipe)) {
                    int l = (int)strlen(pbuf);
                    while (l > 0 && (pbuf[l - 1] == '\r' || pbuf[l - 1] == '\n')) l--;
                    insert_row(ins_line + added, pbuf, l);
                    added++;
                }
                _pclose(pipe);
                set_status("\"%s\" %d lines", r_arg + 1, added);
            }
        } else {
            FILE *fp = fopen(r_arg, "r");
            if (fp) {
                save_undo();
                char fbuf[4096];
                int added = 0;
                while (fgets(fbuf, sizeof(fbuf), fp)) {
                    int l = (int)strlen(fbuf);
                    while (l > 0 && (fbuf[l - 1] == '\r' || fbuf[l - 1] == '\n')) l--;
                    insert_row(ins_line + added, fbuf, l);
                    added++;
                }
                fclose(fp);
                set_status("\"%s\" %d lines", r_arg, added);
            } else {
                set_status("E484: Can't open file %s", r_arg);
            }
        }
    } else if (strncmp(cmd, "set", 3) == 0 && (cmd[3] == ' ' || cmd[3] == '\0')) {
        execute_set_command(cmd + 3);
    } else if (strcmp(cmd, "syntax on") == 0 || strcmp(cmd, "syn on") == 0) {
        E.opt.syntax = true;
    } else if (strcmp(cmd, "syntax off") == 0 || strcmp(cmd, "syn off") == 0) {
        E.opt.syntax = false;
    } else if (strcmp(cmd, "noh") == 0 || strcmp(cmd, "nohl") == 0 || strcmp(cmd, "nohlsearch") == 0) {
        E.opt.nohl_active = true;
    } else if (strncmp(cmd, "global", 6) == 0 && (cmd[6] == ' ' || !isalpha((unsigned char)cmd[6]))) {
        int r1 = has_range ? start : 1;
        int r2 = has_range ? end : E.num_lines;
        execute_global(r1, r2, false, cmd + 6);
    } else if (strncmp(cmd, "vglobal", 7) == 0 && (cmd[7] == ' ' || !isalpha((unsigned char)cmd[7]))) {
        int r1 = has_range ? start : 1;
        int r2 = has_range ? end : E.num_lines;
        execute_global(r1, r2, true, cmd + 7);
    } else if (*cmd == 'g' && *(cmd + 1) == '!') {
        int r1 = has_range ? start : 1;
        int r2 = has_range ? end : E.num_lines;
        execute_global(r1, r2, true, cmd + 2);
    } else if (*cmd == 'g' && (cmd[1] == ' ' || !isalpha((unsigned char)cmd[1]))) {
        int r1 = has_range ? start : 1;
        int r2 = has_range ? end : E.num_lines;
        execute_global(r1, r2, false, cmd + 1);
    } else if (*cmd == 'v' && (cmd[1] == ' ' || !isalpha((unsigned char)cmd[1]))) {
        int r1 = has_range ? start : 1;
        int r2 = has_range ? end : E.num_lines;
        execute_global(r1, r2, true, cmd + 1);
    } else if (strncmp(cmd, "move", 4) == 0 && (cmd[4] == ' ' || !isalpha((unsigned char)cmd[4]))) {
        int r1 = has_range ? start : E.cy + 1;
        int r2 = has_range ? end : E.cy + 1;
        execute_move_or_copy(r1, r2, cmd + 4, true);
    } else if (*cmd == 'm' && (cmd[1] == ' ' || !isalpha((unsigned char)cmd[1]))) {
        int r1 = has_range ? start : E.cy + 1;
        int r2 = has_range ? end : E.cy + 1;
        execute_move_or_copy(r1, r2, cmd + 1, true);
    } else if (strncmp(cmd, "copy", 4) == 0 && (cmd[4] == ' ' || !isalpha((unsigned char)cmd[4]))) {
        int r1 = has_range ? start : E.cy + 1;
        int r2 = has_range ? end : E.cy + 1;
        execute_move_or_copy(r1, r2, cmd + 4, false);
    } else if (strncmp(cmd, "co", 2) == 0 && (cmd[2] == ' ' || !isalpha((unsigned char)cmd[2]))) {
        int r1 = has_range ? start : E.cy + 1;
        int r2 = has_range ? end : E.cy + 1;
        execute_move_or_copy(r1, r2, cmd + 2, false);
    } else if (*cmd == 't' && (cmd[1] == ' ' || !isalpha((unsigned char)cmd[1]))) {
        int r1 = has_range ? start : E.cy + 1;
        int r2 = has_range ? end : E.cy + 1;
        execute_move_or_copy(r1, r2, cmd + 1, false);
    } else if (*cmd == 's' && (cmd[1] == ' ' || !isalpha((unsigned char)cmd[1]))) {
        int r1 = has_range ? start : E.cy + 1;
        int r2 = has_range ? end : E.cy + 1;
        execute_substitute(r1, r2, cmd + 1);
    } else if (strcmp(cmd, "d") == 0) {
        int r1 = has_range ? start : E.cy + 1;
        int r2 = has_range ? end : E.cy + 1;
        apply_operator_on_range('d', r1 - 1, 0, r2 - 1, 0, true);
    } else if (strcmp(cmd, "y") == 0) {
        int r1 = has_range ? start : E.cy + 1;
        int r2 = has_range ? end : E.cy + 1;
        apply_operator_on_range('y', r1 - 1, 0, r2 - 1, 0, true);
    } else if (strcmp(cmd, "n") == 0 || strcmp(cmd, "next") == 0) {
        if (E.dirty) {
            set_status("E37: No write since last change (add ! to override)");
        } else if (E.cur_arg + 1 < E.num_args) {
            E.cur_arg++;
            editor_open(E.args_files[E.cur_arg]);
        } else {
            set_status("No more files to edit");
        }
    } else if (strcmp(cmd, "n!") == 0 || strcmp(cmd, "next!") == 0) {
        if (E.cur_arg + 1 < E.num_args) {
            E.cur_arg++;
            editor_open(E.args_files[E.cur_arg]);
        } else {
            set_status("No more files to edit");
        }
    } else if (strcmp(cmd, "prev") == 0 || strcmp(cmd, "N") == 0) {
        if (E.dirty) {
            set_status("E37: No write since last change (add ! to override)");
        } else if (E.cur_arg > 0) {
            E.cur_arg--;
            editor_open(E.args_files[E.cur_arg]);
        } else {
            set_status("Already on first file");
        }
    } else if (strcmp(cmd, "prev!") == 0 || strcmp(cmd, "N!") == 0) {
        if (E.cur_arg > 0) {
            E.cur_arg--;
            editor_open(E.args_files[E.cur_arg]);
        } else {
            set_status("Already on first file");
        }
    } else if (strcmp(cmd, "args") == 0) {
        char abuf[256] = {0};
        int ptr = 0;
        for (int i = 0; i < E.num_args && ptr < (int)sizeof(abuf) - 30; i++) {
            if (i == E.cur_arg) ptr += snprintf(&abuf[ptr], sizeof(abuf) - ptr, "[%s] ", E.args_files[i]);
            else ptr += snprintf(&abuf[ptr], sizeof(abuf) - ptr, "%s ", E.args_files[i]);
        }
        set_status("%s", abuf);
    } else if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0) {
        E.mode = MODE_HELP;
        E.help_scroll = 0;
    } else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "ve") == 0) {
        set_status("%s", VI_VERSION);
    } else if (strcmp(cmd, "Q") == 0) {
        E.mode = MODE_EX;
    } else if (isdigit((unsigned char)*cmd)) {
        int line_no = atoi(cmd);
        E.cy = max(0, min(E.num_lines - 1, line_no - 1));
        E.cx = 0;
    } else {
        set_status("E492: Not an editor command: %s", cmd);
    }
}

void run_ex_mode(void) {
    printf("\r\nEntering Ex mode. Type 'visual' or 'vi' to return to visual mode.\r\n");
    char line[512];
    while (E.mode == MODE_EX) {
        printf(":");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        int len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) line[--len] = '\0';
        if (strcmp(line, "vi") == 0 || strcmp(line, "visual") == 0) {
            E.mode = MODE_NORMAL;
            break;
        }
        execute_ex_command(line);
        if (E.status_msg[0]) {
            printf("%s\r\n", E.status_msg);
            E.status_msg[0] = '\0';
        }
    }
}

void load_virc(void) {
    char path[MAX_PATH];
    const char *home = getenv("USERPROFILE");
    if (!home) home = getenv("HOME");

    FILE *fp = NULL;
    if (home) {
        snprintf(path, sizeof(path), "%s\\_virc", home);
        fp = fopen(path, "r");
        if (!fp) {
            snprintf(path, sizeof(path), "%s\\.virc", home);
            fp = fopen(path, "r");
        }
        if (!fp) {
            snprintf(path, sizeof(path), "%s\\_vimrc", home);
            fp = fopen(path, "r");
        }
        if (!fp) {
            snprintf(path, sizeof(path), "%s\\.exrc", home);
            fp = fopen(path, "r");
        }
    }
    if (!fp) {
        fp = fopen("_virc", "r");
        if (!fp) fp = fopen(".virc", "r");
    }
    if (!fp) return;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        int l = (int)strlen(line);
        while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == '\n')) line[--l] = '\0';
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '"' || *p == '#') continue;
        if (*p == ':') p++;
        char *comment = strchr(p, '"');
        if (comment) *comment = '\0';
        int plen = (int)strlen(p);
        while (plen > 0 && (p[plen - 1] == ' ' || p[plen - 1] == '\t')) p[--plen] = '\0';
        if (*p == '\0') continue;
        execute_ex_command(p);
    }
    fclose(fp);
    E.status_msg[0] = '\0';
}
