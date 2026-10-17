#include "vi.h"
#include "screen.h"
#include "term.h"
#include "search.h"
#include "syntax.h"
#include "diff.h"
#include "tab.h"
#include "fold.h"
#include "quickfix.h"

static const char *HELP_TEXT[] = {
    "================================================================================",
    "                         VI USER MANUAL (Vi 5.0.0-2026)                         ",
    "================================================================================",
    "",
    "Vi is a modal, grammar-driven text editor adhering to classic POSIX / BSD vi.",
    "Commands follow the grammar: [count][operator][count][motion]",
    "",
    "--------------------------------------------------------------------------------",
    "1. MODES & TRANSITIONS",
    "--------------------------------------------------------------------------------",
    "  NORMAL mode:  Default navigation & command mode.",
    "  VISUAL mode:  Select text interactively. Enter via 'v' (char), 'V' (line), Ctrl-V (block).",
    "  INSERT mode:  Type text into buffer. Enter via: i, I, a, A, o, O, s, S, c, C.",
    "  REPLACE mode: Overstrike text. Enter via 'R'. Exit with <ESC>.",
    "  COMMAND mode: Enter via ':' to run ex commands.",
    "  SEARCH mode:  Enter via '/' (forward) or '?' (backward). Live incremental search.",
    "  EX mode:      Enter via 'Q' to access interactive line editor. Type 'vi' to return.",
    "  HELP mode:    Enter via ':help' or <F1>. Exit with 'q' or <ESC>.",
    "",
    "--------------------------------------------------------------------------------",
    "2. MOTIONS (Accept count, e.g. 5w, 3j, 10G)",
    "--------------------------------------------------------------------------------",
    "  h, j, k, l       Left, Down, Up, Right (Arrows also supported; UTF-8 aware)",
    "  0, ^, $          Start of line, First non-blank char, End of line",
    "  w, W             Forward to start of next word / WORD (space delimited)",
    "  b, B             Backward to start of previous word / WORD",
    "  e, E             Forward to end of word / WORD",
    "  f{c}, F{c}       Find char {c} forward / backward on current line",
    "  t{c}, T{c}       Till char {c} forward / backward on current line",
    "  ; and ,          Repeat last f/F/t/T in same / opposite direction",
    "  G, gg            Go to line [count]G (default: EOF); gg = line 1",
    "  + / <CR>, -      First non-blank on next line / previous line",
    "  |                Jump to screen column [count]|",
    "  %                Jump to matching bracket: (), {}, []",
    "  H, M, L          High (top), Middle, Low (bottom) line of current screen",
    "  Ctrl-F, Ctrl-B   Scroll page down / up",
    "  Ctrl-D, Ctrl-U   Scroll half-page down / up",
    "  Ctrl-E, Ctrl-Y   Scroll screen down / up 1 line without moving cursor",
    "  z<CR>, z., z-    Redraw screen with cursor line at Top, Center, Bottom",
    "  m{a-z}           Set mark {a-z}",
    "  '{a-z}, `{a-z}   Jump to mark line / exact position ('' = previous jump)",
    "",
    "--------------------------------------------------------------------------------",
    "3. OPERATORS, TEXT OBJECTS & EDITING",
    "--------------------------------------------------------------------------------",
    "  d{motion}, dd    Delete (cut) over motion; dd deletes current line",
    "  c{motion}, cc    Change over motion; cc changes whole line (enters insert)",
    "  y{motion}, yy, Y Yank (copy) over motion; yy / Y yanks whole line",
    "  >{motion}, >>    Shift right (indent) lines by shiftwidth",
    "  <{motion}, <<    Shift left (unindent) lines by shiftwidth",
    "  x, X             Delete character under / before cursor (UTF-8 safe)",
    "  s, S             Substitute char (delete & insert) / line (same as cc)",
    "  r{c}             Replace character under cursor with {c}",
    "  R                Enter REPLACE (overstrike) mode",
    "  C, D             Change to end of line (c$) / Delete to end of line (d$)",
    "  J                Join current line with next line (normalizes spaces)",
    "  ~                Toggle case of character under cursor and advance",
    "  p, P             Put (paste) register contents after / before cursor",
    "  \"[a-z]{op}      Use named register [a-z]; \"+ / \"* for Windows Clipboard",
    "  q{a-z}           Record macro to register; press 'q' to stop recording",
    "  @{a-z}, @@       Play macro in register / repeat last macro with [count]",
    "  u, Ctrl-R        Undo / Redo changes (multi-level stack; 1-level via :set cp)",
    "  U                Restore current line to state before cursor moved to it",
    "  Ctrl-N, Ctrl-P   In INSERT mode: Keyword completion (cycle next / prev match)",
    "  .                Repeat last modifying command with [count]",
    "  Ctrl-G           Display file status: name, modified, line count, %",
    "",
    "  TEXT OBJECTS (usable with d, c, y, v):",
    "    iw / aw        Inner / Around Word",
    "    i\" / a\"        Inner / Around Double Quotes",
    "    i' / a'        Inner / Around Single Quotes",
    "    i` / a`        Inner / Around Backticks",
    "    i( / a(        Inner / Around Parentheses (also ib / ab)",
    "    i[ / a[        Inner / Around Square Brackets",
    "    i{ / a{        Inner / Around Curly Brackets (also iB / aB)",
    "    i< / a<        Inner / Around Angle Brackets",
    "",
    "  VISUAL BLOCK MODE (Ctrl-V or Ctrl-Q):",
    "    I{text}<ESC>   Insert text at start column across all selected lines",
    "    A{text}<ESC>   Append text at end column across all selected lines",
    "    d, x, y, c     Delete, cut, yank, or change rectangular block",
    "",
    "--------------------------------------------------------------------------------",
    "4. MULTI-WINDOW SPLITS & BUFFER MANAGEMENT",
    "--------------------------------------------------------------------------------",
    "  :split, :sp      Split window horizontally (Ctrl-W s)",
    "  :vsplit, :vs     Split window vertically (Ctrl-W v)",
    "  :close, :clo     Close current window (Ctrl-W c)",
    "  :only, :on       Close all other windows except current (Ctrl-W o)",
    "  Ctrl-W w         Move cursor focus to next split window",
    "  Ctrl-W p         Move cursor focus to previous split window",
    "  Ctrl-W h/j/k/l   Move focus Left, Down, Up, Right between windows",
    "  Ctrl-W =         Balance all split window heights equally",
    "  :ls, :buffers    List all open in-memory buffers (ID, %, +, a, filename)",
    "  :b <id|name>     Switch to buffer by numeric ID or filename substring",
    "  :bn, :bp         Switch to Next / Previous buffer in RAM",
    "  :bd, :bd!        Delete (close) current buffer (! forces unsaved buffer)",
    "  :badd <file>     Add <file> to buffer table for instant switching",
    "  :recover         Restore buffer from real-time crash journal (.{file}.swp)",
    "",
    "--------------------------------------------------------------------------------",
    "5. CTAGS, DIRECTORY EXPLORER & EX COMMANDS",
    "--------------------------------------------------------------------------------",
    "  Ctrl-]           Jump to CTags symbol definition under cursor (:tag <sym>)",
    "  Ctrl-T           Pop back from CTags jump (:pop)",
    "  :tags            Display active CTags jump stack",
    "  :Explore, :E     Open Directory Explorer in current window",
    "  :Sex             Open Directory Explorer in new horizontal split",
    "  vi <dir>, :e <dir> Open directory directly in Directory Explorer",
    "  :[range]!<cmd>   Filter range through external shell (e.g. :%!sort)",
    "  :[range]g/pat/cmd Execute cmd on matching lines (default: %)",
    "  :[range]v/pat/cmd Execute cmd on non-matching lines (:g!)",
    "  :[range]s/find/replace/[g][i]   Substitute in range (% = all, . = current)",
    "  :[range]d, y     Delete / Yank lines in range",
    "  :[range]co, m <d> Copy / Move lines in range to destination line",
    "  :w, :q, :wq, :x  Save buffer / Quit / Save and quit",
    "  :e [file], :e!   Open file in buffer / revert changes",
    "  :r [file|!cmd]   Read file or shell output into buffer",
    "  :colorscheme <n> Set theme: dark, light, monokai, gruvbox, classic (or :colo)",
    "  :set [nu|nonu]   Toggle line numbers",
    "  :set [ic|noic]   Toggle ignore-case search",
    "  :set [is|nois]   Toggle incremental search (incsearch)",
    "  :set [wrap|nowrap] Toggle soft line wrapping",
    "  :set [hls|nohls] Toggle search match highlighting (:nohl to clear)",
    "  :set [syn|nosyn] Toggle syntax highlighting",
    "  :set [nocp|cp]   Toggle multi-level undo vs 1-level toggle",
    "  :set [ai|noai]   Toggle auto-indenting",
    "  :set [sm|nosm]   Toggle showmode indicator",
    "  :set [ruler|noruler] Toggle status bar display",
    "  :set ff=[dos|unix]  Toggle CRLF (DOS) / LF (UNIX) format",
    "  :set ts=N, sw=N  Set tabstop / shiftwidth spaces",
    "  :Q               Enter interactive Ex line editor",
    "",
    "--------------------------------------------------------------------------------",
    "6. SUPPORTED SYNTAX HIGHLIGHTING (Automatic filetype detection)",
    "--------------------------------------------------------------------------------",
    "  C / C++       .c, .h, .cpp, .hpp, .cc, .cxx",
    "  Rust          .rs (keywords, types, comments, #[attribute] macros)",
    "  Go            .go (keywords, types, comments, `raw string` literals)",
    "  Basic         .bas, .vbs, .vb (case-insensitive keywords, types, ', REM)",
    "  PowerShell    .ps1, .psm1, .psd1 (cmdlets, keywords, $variables)",
    "  Batch / CMD   .bat, .cmd (commands, REM, :: comments)",
    "  Python        .py (def, class, imports, strings, # comments)",
    "  Shell         .sh, .bash, .zsh",
    "  JSON          .json",
    "  Markdown      .md, .markdown (headings, formatting)",
    "",
    "Press 'q' or <ESC> to exit Help and return to editing.",
    "================================================================================",
    NULL
};

void set_status(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.status_msg, sizeof(E.status_msg), fmt, ap);
    va_end(ap);
    E.status_msg_time = GetTickCount();
}

int row_cx_to_rx(Line *row, int cx) {
    int rx = 0;
    for (int j = 0; j < cx && j < row->len; j++) {
        if (row->chars[j] == '\t')
            rx += (E.opt.tabstop - (rx % E.opt.tabstop));
        else
            rx++;
    }
    return rx;
}

void editor_scroll(void) {
    if (E.cy >= 0 && E.cy < E.num_lines && fold_is_line_hidden(E.cy)) {
        Fold *f = fold_find_at(E.cy);
        if (f && f->collapsed) E.cy = f->start_y;
    }

    E.screen_x = 0;
    if (E.cy < E.num_lines) {
        E.screen_x = row_cx_to_rx(&E.lines[E.cy], E.cx);
    }

    int win_rows = E.screen_rows;
    int win_cols = E.screen_cols;
    if (E.cur_win_idx >= 0 && E.cur_win_idx < E.num_windows) {
        win_rows = E.windows[E.cur_win_idx].rows;
        win_cols = E.windows[E.cur_win_idx].cols;
    }

    int num_w = E.opt.number ? 7 : 0;
    int usable_cols = win_cols - num_w;
    if (usable_cols < 1) usable_cols = 1;

    if (E.opt.wrap) {
        E.col_off = 0;
        if (E.cy < E.row_off) E.row_off = E.cy;
        int cur_screen_y = 0;
        for (int r = E.row_off; r < E.cy && r < E.num_lines; r++) {
            int rlen = E.lines[r].len;
            cur_screen_y += (rlen <= usable_cols) ? 1 : ((rlen + usable_cols - 1) / usable_cols);
        }
        int chunk_offset = (usable_cols > 0) ? (E.cx / usable_cols) : 0;
        int total_offset = cur_screen_y + chunk_offset;
        while (total_offset >= win_rows && E.row_off < E.cy) {
            int rlen = E.lines[E.row_off].len;
            int chunks = (rlen <= usable_cols) ? 1 : ((rlen + usable_cols - 1) / usable_cols);
            total_offset -= chunks;
            E.row_off++;
        }
    } else {
        if (E.cy < E.row_off) E.row_off = E.cy;
        while (E.row_off < E.cy && fold_is_line_hidden(E.row_off)) E.row_off++;
        int vis_rows = 0;
        for (int r = E.row_off; r <= E.cy && r < E.num_lines; r++) {
            if (!fold_is_line_hidden(r)) vis_rows++;
        }
        while (vis_rows > win_rows && E.row_off < E.cy) {
            if (!fold_is_line_hidden(E.row_off)) vis_rows--;
            E.row_off++;
            while (E.row_off < E.cy && fold_is_line_hidden(E.row_off)) E.row_off++;
        }
        if (E.screen_x < E.col_off) E.col_off = E.screen_x;
        if (E.screen_x >= E.col_off + usable_cols) E.col_off = E.screen_x - usable_cols + 1;
    }
}

typedef struct {
    char *b;
    int len;
    int cap;
} AppendBuf;

static void ab_append(AppendBuf *ab, const char *s, int len) {
    if (ab->len + len >= ab->cap) {
        int new_cap = (ab->cap == 0) ? 16384 : (ab->cap + len + 16384) * 2;
        ab->b = (char *)realloc(ab->b, new_cap);
        ab->cap = new_cap;
    }
    memcpy(&ab->b[ab->len], s, len);
    ab->len += len;
}

static void ab_appends(AppendBuf *ab, const char *s) {
    ab_append(ab, s, (int)strlen(s));
}

static void ab_printf(AppendBuf *ab, const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) ab_append(ab, buf, n);
}

static void ab_free(AppendBuf *ab) {
    free(ab->b);
    ab->b = NULL;
    ab->len = 0;
    ab->cap = 0;
}

void render_help(void) {
    int count = 0;
    while (HELP_TEXT[count]) count++;

    AppendBuf ab = {0};
    ab_appends(&ab, "\x1b[?2026h\x1b[?25l\x1b[H");

    for (int y = 0; y < E.screen_rows; y++) {
        int help_idx = E.help_scroll + y;
        if (help_idx < count) {
            char line[256];
            strncpy(line, HELP_TEXT[help_idx], sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';
            if ((int)strlen(line) > E.screen_cols) line[E.screen_cols] = '\0';
            ab_appends(&ab, line);
        } else {
            ab_appends(&ab, "~");
        }
        ab_appends(&ab, "\x1b[K\r\n");
    }

    ab_printf(&ab, "\x1b[7m-- VI MANUAL (Line %d of %d) - Press 'q' or <ESC> to Exit --\x1b[0m\x1b[K\r\n",
              E.help_scroll + 1, count);
    ab_appends(&ab, "Navigation: [j] Down | [k] Up | [q] Exit Help\x1b[K");
    ab_appends(&ab, "\x1b[?25h\x1b[?2026l");

    if (E.hOut && E.hOut != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(E.hOut, ab.b, ab.len, &written, NULL);
    } else {
        fwrite(ab.b, 1, ab.len, stdout);
        fflush(stdout);
    }
    ab_free(&ab);
}

static bool is_in_visual_selection(int y, int x) {
    if (E.mode != MODE_VISUAL && E.mode != MODE_VISUAL_LINE && E.mode != MODE_VISUAL_BLOCK)
        return false;

    int y1 = E.visual_anchor_y, x1 = E.visual_anchor_x;
    int y2 = E.cy, x2 = E.cx;

    if (E.mode == MODE_VISUAL_LINE) {
        int minY = min(y1, y2), maxY = max(y1, y2);
        return (y >= minY && y <= maxY);
    }

    if (E.mode == MODE_VISUAL_BLOCK) {
        int minY = min(y1, y2), maxY = max(y1, y2);
        int minX = min(x1, x2), maxX = max(x1, x2);
        return (y >= minY && y <= maxY && x >= minX && x <= maxX);
    }

    /* MODE_VISUAL (characterwise) */
    if (y1 > y2 || (y1 == y2 && x1 > x2)) {
        int ty = y1; y1 = y2; y2 = ty;
        int tx = x1; x1 = x2; x2 = tx;
    }

    if (y < y1 || y > y2) return false;
    if (y1 == y2) return (x >= x1 && x <= x2);
    if (y == y1) return (x >= x1);
    if (y == y2) return (x <= x2);
    return true;
}

static void render_tabline(AppendBuf *ab) {
    if (E.num_tabs <= 1) return;
    ab_printf(ab, "\x1b[1;1H");

    int printed = 0;
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

        if (printed + tlen >= E.screen_cols) break;

        if (is_cur) {
            ab_printf(ab, "\x1b[7m%s\x1b[0m", tab_text);
        } else {
            ab_printf(ab, "\x1b[100;90m%s\x1b[0m", tab_text);
        }
        printed += tlen;
    }

    ab_appends(ab, "\x1b[100;37m");
    while (printed < E.screen_cols) {
        ab_append(ab, " ", 1);
        printed++;
    }
    ab_appends(ab, "\x1b[0m");
}

static void render_window(AppendBuf *ab, int win_idx, bool is_active) {
    EditorWindow *w = &E.windows[win_idx];
    int num_w = E.opt.number ? 7 : (w->diff_mode ? 2 : 0);
    int usable_cols = w->cols - num_w;
    if (usable_cols < 1) usable_cols = 1;

    int win_row_off = is_active ? E.row_off : w->row_off;
    int win_col_off = is_active ? E.col_off : w->col_off;

    Line *lines = E.lines;
    int num_lines = E.num_lines;
    if (!is_active && w->buf_idx >= 0 && w->buf_idx < E.num_buffers && w->buf_idx != E.cur_buf_idx && E.buffers[w->buf_idx].loaded) {
        lines = E.buffers[w->buf_idx].lines;
        num_lines = E.buffers[w->buf_idx].num_lines;
    }

    int cur_r = win_row_off;
    int cur_chunk = 0;

    for (int y = 0; y < w->rows; y++) {
        ab_printf(ab, "\x1b[%d;%dH", w->top + y + 1, w->left + 1);
        int printed_cols = 0;

        if (E.opt.wrap) {
            if (cur_r >= num_lines) {
                if (E.opt.number) { ab_appends(ab, "       ~"); printed_cols += 8; }
                else { ab_appends(ab, "~"); printed_cols += 1; }
            } else {
                int row_len = lines[cur_r].len;
                if (cur_chunk == 0 && E.opt.number) {
                    ab_printf(ab, "\x1b[33m%6d \x1b[0m", cur_r + 1);
                    printed_cols += 7;
                } else if (cur_chunk > 0 && E.opt.number) {
                    ab_appends(ab, "       ");
                    printed_cols += 7;
                }

                int chunk_start = cur_chunk * usable_cols;
                int chunk_len = row_len - chunk_start;
                if (chunk_len > usable_cols) chunk_len = usable_cols;
                if (chunk_len < 0) chunk_len = 0;

                if (chunk_len > 0) {
                    char *hl_mask = NULL;
                    if (E.opt.hlsearch && !E.opt.nohl_active && E.last_search[0] != '\0' && row_len > 0) {
                        hl_mask = (char *)calloc(row_len + 1, sizeof(char));
                        const char *src = lines[cur_r].chars;
                        int mlen = 0;
                        while (*src) {
                            const char *m = regex_search_line(E.last_search, src, E.opt.ignorecase, &mlen);
                            if (!m) break;
                            int idx = (int)(m - lines[cur_r].chars);
                            if (mlen <= 0) mlen = 1;
                            for (int k = 0; k < mlen && idx + k < row_len; k++) hl_mask[idx + k] = 1;
                            src = m + max(1, mlen);
                        }
                    }

                    unsigned char *hl_syntax = NULL;
                    if (E.opt.syntax && row_len > 0 && (is_active || w->buf_idx == E.cur_buf_idx)) {
                        hl_syntax = (unsigned char *)malloc(row_len + 1);
                        syntax_highlight_row(cur_r, hl_syntax);
                    }

                    char *slice = &lines[cur_r].chars[chunk_start];
                    int current_style = -1;
                    for (int i = 0; i < chunk_len; i++) {
                        int char_idx = chunk_start + i;
                        bool in_vis = is_active && is_in_visual_selection(cur_r, char_idx);
                        bool in_search = (hl_mask && char_idx < row_len && hl_mask[char_idx]);
                        int syn = (hl_syntax && char_idx < row_len) ? hl_syntax[char_idx] : HL_NORMAL;

                        int target_style = in_vis ? -2 : (in_search ? -3 : syn);
                        if (target_style != current_style) {
                            current_style = target_style;
                            if (current_style == -2) ab_appends(ab, "\x1b[7m");
                            else if (current_style == -3) ab_appends(ab, "\x1b[43;30m");
                            else ab_printf(ab, "\x1b[0m%s", syntax_color_code((unsigned char)current_style));
                        }
                        ab_append(ab, &slice[i], 1);
                        printed_cols++;
                    }
                    ab_appends(ab, "\x1b[0m");

                    if (hl_mask) free(hl_mask);
                    if (hl_syntax) free(hl_syntax);
                }

                cur_chunk++;
                if (cur_chunk * usable_cols >= row_len || row_len == 0) {
                    cur_r++;
                    cur_chunk = 0;
                }
            }
        } else {
            while ((is_active || w->buf_idx == E.cur_buf_idx) && cur_r < num_lines && fold_is_line_hidden(cur_r)) {
                cur_r++;
            }
            int filerow = cur_r;
            if (filerow >= num_lines) {
                if (E.opt.number) { ab_appends(ab, "       ~"); printed_cols += 8; }
                else { ab_appends(ab, "~"); printed_cols += 1; }
            } else {
            Fold *fstart = (is_active || w->buf_idx == E.cur_buf_idx) ? fold_find_start(filerow) : NULL;
            if (fstart) {
                char banner[256];
                int blen = snprintf(banner, sizeof(banner), "+-- %d lines folded ----------------------------------------------------",
                                    fstart->end_y - fstart->start_y + 1);
                if (blen > usable_cols) blen = usable_cols;
                ab_printf(ab, "\x1b[36;7m%.*s\x1b[0m", blen, banner);
                printed_cols += blen;
                cur_r = fstart->end_y + 1;
            } else {
                cur_r++;
                int diff_st = w->diff_mode ? diff_get_status(w->buf_idx, filerow) : DIFF_NONE;
                if (E.opt.number) {
                    if (diff_st == DIFF_ADD) {
                        ab_printf(ab, "\x1b[32m%5d+ \x1b[0m", filerow + 1);
                    } else if (diff_st == DIFF_DEL) {
                        ab_printf(ab, "\x1b[31m%5d- \x1b[0m", filerow + 1);
                    } else if (diff_st == DIFF_CHANGE) {
                        ab_printf(ab, "\x1b[33m%5d! \x1b[0m", filerow + 1);
                    } else {
                        ab_printf(ab, "\x1b[33m%6d \x1b[0m", filerow + 1);
                    }
                    printed_cols += 7;
                } else if (w->diff_mode) {
                    if (diff_st == DIFF_ADD) { ab_appends(ab, "\x1b[32m+ \x1b[0m"); printed_cols += 2; }
                    else if (diff_st == DIFF_DEL) { ab_appends(ab, "\x1b[31m- \x1b[0m"); printed_cols += 2; }
                    else if (diff_st == DIFF_CHANGE) { ab_appends(ab, "\x1b[33m! \x1b[0m"); printed_cols += 2; }
                    else { ab_appends(ab, "  "); printed_cols += 2; }
                }

                int row_len = lines[filerow].len;
                char *hl_mask = NULL;
                if (E.opt.hlsearch && !E.opt.nohl_active && E.last_search[0] != '\0' && row_len > 0) {
                    hl_mask = (char *)calloc(row_len + 1, sizeof(char));
                    const char *src = lines[filerow].chars;
                    int mlen = 0;
                    while (*src) {
                        const char *m = regex_search_line(E.last_search, src, E.opt.ignorecase, &mlen);
                        if (!m) break;
                        int idx = (int)(m - lines[filerow].chars);
                        if (mlen <= 0) mlen = 1;
                        for (int k = 0; k < mlen && idx + k < row_len; k++) hl_mask[idx + k] = 1;
                        src = m + max(1, mlen);
                    }
                }

                unsigned char *hl_syntax = NULL;
                if (E.opt.syntax && row_len > 0 && (is_active || w->buf_idx == E.cur_buf_idx)) {
                    hl_syntax = (unsigned char *)malloc(row_len + 1);
                    syntax_highlight_row(filerow, hl_syntax);
                }

                int len = row_len - win_col_off;
                if (len < 0) len = 0;
                if (len > usable_cols) len = usable_cols;
                if (len > 0) {
                    char *slice = &lines[filerow].chars[win_col_off];
                    int current_style = -1;
                    int def_style = HL_NORMAL;
                    if (diff_st == DIFF_ADD) def_style = HL_DIFF_ADD;
                    else if (diff_st == DIFF_DEL) def_style = HL_DIFF_DELETE;
                    else if (diff_st == DIFF_CHANGE) def_style = HL_DIFF_CHANGE;

                    for (int i = 0; i < len; i++) {
                        int char_idx = win_col_off + i;
                        bool in_vis = is_active && is_in_visual_selection(filerow, char_idx);
                        bool in_search = (hl_mask && char_idx < row_len && hl_mask[char_idx]);
                        int syn = (hl_syntax && char_idx < row_len) ? hl_syntax[char_idx] : def_style;
                        if (syn == HL_NORMAL && def_style != HL_NORMAL) syn = def_style;

                        int target_style = in_vis ? -2 : (in_search ? -3 : syn);
                        if (target_style != current_style) {
                            current_style = target_style;
                            if (current_style == -2) ab_appends(ab, "\x1b[7m");
                            else if (current_style == -3) ab_appends(ab, "\x1b[43;30m");
                            else ab_printf(ab, "\x1b[0m%s", syntax_color_code((unsigned char)current_style));
                        }
                        ab_append(ab, &slice[i], 1);
                        printed_cols++;
                    }
                    ab_appends(ab, "\x1b[0m");
                }
                if (E.opt.list && printed_cols < w->cols) {
                    ab_appends(ab, "\x1b[90m$\x1b[0m");
                    printed_cols++;
                }
                if (hl_mask) free(hl_mask);
                if (hl_syntax) free(hl_syntax);
            }
            }
        }

        if (printed_cols < w->cols) {
            if (w->left + w->cols >= E.screen_cols) {
                ab_appends(ab, "\x1b[K");
            } else {
                while (printed_cols < w->cols) {
                    ab_append(ab, " ", 1);
                    printed_cols++;
                }
            }
        }
    }

    if (E.num_windows > 1 && (w->top + w->rows < E.screen_rows)) {
        ab_printf(ab, "\x1b[%d;%dH\x1b[7m", w->top + w->rows + 1, w->left + 1);
        char win_title[256];
        const char *fname = (is_active ? E.filename :
                            (w->buf_idx >= 0 && w->buf_idx < E.num_buffers ? E.buffers[w->buf_idx].filename : NULL));
        bool is_dirty = (is_active ? E.dirty : (w->buf_idx >= 0 && w->buf_idx < E.num_buffers ? E.buffers[w->buf_idx].dirty : false));
        bool is_ro = (is_active ? E.readonly : (w->buf_idx >= 0 && w->buf_idx < E.num_buffers ? E.buffers[w->buf_idx].readonly : false));
        int title_len = snprintf(win_title, sizeof(win_title), " [%d] %s%s%s %d,%d",
                                 w->id,
                                 fname ? fname : "[No Name]",
                                 is_dirty ? " [+]" : "",
                                 is_ro ? " [RO]" : "",
                                 (is_active ? E.cy : w->cy) + 1,
                                 (is_active ? E.cx : w->cx) + 1);
        if (title_len > w->cols) title_len = w->cols;
        ab_append(ab, win_title, title_len);
        for (int p = title_len; p < w->cols; p++) ab_append(ab, " ", 1);
        ab_appends(ab, "\x1b[0m");
    }

    if (w->left + w->cols < E.screen_cols) {
        int sep_col = w->left + w->cols + 1;
        for (int y = 0; y < w->rows; y++) {
            ab_printf(ab, "\x1b[%d;%dH\x1b[90m|\x1b[0m", w->top + y + 1, sep_col);
        }
        if (w->top + w->rows < E.screen_rows) {
            ab_printf(ab, "\x1b[%d;%dH\x1b[90m+\x1b[0m", w->top + w->rows + 1, sep_col);
        }
    }
}

void editor_refresh_screen(void) {
    if (E.mode == MODE_HELP) {
        render_help();
        return;
    }
    if (E.mode == MODE_EX) return;

    term_get_window_size();
    int top_offset = (E.num_tabs > 1) ? 1 : 0;
    int avail_rows = E.screen_rows - top_offset;
    if (E.num_windows <= 1) {
        E.windows[0].top = top_offset;
        E.windows[0].left = 0;
        E.windows[0].rows = avail_rows;
        E.windows[0].cols = E.screen_cols;
        E.num_windows = 1;
        E.cur_win_idx = 0;
    }
    quickfix_check_completed();
    diff_sync_scroll();
    editor_scroll();

    AppendBuf ab = {0};
    ab_appends(&ab, "\x1b[?2026h\x1b[?25l\x1b[H");

    if (E.num_tabs > 1) {
        render_tabline(&ab);
    }

    for (int i = 0; i < E.num_windows; i++) {
        render_window(&ab, i, i == E.cur_win_idx);
    }

    if (E.num_windows == 1 && E.opt.ruler) {
        ab_printf(&ab, "\x1b[%d;1H\x1b[7m", E.screen_rows + 1);
        char status[80], rstatus[80];
        int len = snprintf(status, sizeof(status), " %s%s%s",
                           E.filename ? E.filename : "[No Name]",
                           E.dirty ? " [+]" : "",
                           E.readonly ? " [RO]" : "");
        int rlen = snprintf(rstatus, sizeof(rstatus), "%d,%d   ", E.cy + 1, E.cx + 1);

        if (len > E.screen_cols) len = E.screen_cols;
        ab_append(&ab, status, len);
        while (len < E.screen_cols) {
            if (E.screen_cols - len == rlen) {
                ab_appends(&ab, rstatus);
                break;
            } else {
                ab_append(&ab, " ", 1);
                len++;
            }
        }
        ab_appends(&ab, "\x1b[0m\r\n");
    }

    int cmd_row = (E.num_windows == 1 && E.opt.ruler) ? E.screen_rows + 2 : E.screen_rows + 1;
    ab_printf(&ab, "\x1b[%d;1H\x1b[K", cmd_row);

    if (E.mode == MODE_COMMAND) {
        ab_printf(&ab, ":%s", E.cmd_buf);
    } else if (E.mode == MODE_SEARCH) {
        ab_printf(&ab, "%c%s", E.search_forward ? '/' : '?', E.cmd_buf);
    } else if (E.status_msg[0] && (GetTickCount() - E.status_msg_time < 5000)) {
        ab_appends(&ab, E.status_msg);
    } else {
        if (E.opt.showmode) {
            if (E.mode == MODE_INSERT) ab_appends(&ab, "-- INSERT -- ");
            else if (E.mode == MODE_REPLACE) ab_appends(&ab, "-- REPLACE -- ");
            else if (E.mode == MODE_VISUAL) ab_appends(&ab, "-- VISUAL -- ");
            else if (E.mode == MODE_VISUAL_LINE) ab_appends(&ab, "-- VISUAL LINE -- ");
            else if (E.mode == MODE_VISUAL_BLOCK) ab_appends(&ab, "-- VISUAL BLOCK -- ");
        }
        if (E.recording_macro) {
            ab_printf(&ab, "recording @%c", E.macro_reg);
        }
    }

    if (quickfix_is_running()) {
        static int spin_idx = 0;
        const char *spin = "|/-\\";
        char pat[64];
        int found = quickfix_get_async_progress(pat, sizeof(pat));
        char spin_str[128];
        int slen = snprintf(spin_str, sizeof(spin_str), "[Grep: \"%s\" %d found %c]",
                            pat, found, spin[(spin_idx++) % 4]);
        ab_printf(&ab, "\x1b[%dG\x1b[33m%s\x1b[0m", max(1, E.screen_cols - slen), spin_str);
    }

    if ((E.count1 > 0 || E.pending_op || E.active_reg) && E.mode == MODE_NORMAL) {
        char pend[32];
        int plen = snprintf(pend, sizeof(pend), "%s%s%d%s",
                            E.active_reg ? "\"" : "",
                            E.active_reg ? (char[]){E.active_reg, 0} : "",
                            E.count1,
                            E.pending_op ? (char[]){E.pending_op, 0} : "");
        ab_printf(&ab, "\x1b[%dG%s", E.screen_cols - plen, pend);
    }

    int cursor_y, cursor_x;
    if (E.mode == MODE_COMMAND || E.mode == MODE_SEARCH) {
        cursor_y = cmd_row;
        cursor_x = E.cmd_len + 2;
    } else {
        EditorWindow *w = &E.windows[E.cur_win_idx];
        int num_w = E.opt.number ? 7 : (w->diff_mode ? 2 : 0);
        int usable_cols = w->cols - num_w;
        if (usable_cols < 1) usable_cols = 1;

        if (E.opt.wrap) {
            int cur_screen_y = 0;
            for (int r = E.row_off; r < E.cy && r < E.num_lines; r++) {
                int rlen = E.lines[r].len;
                cur_screen_y += (rlen <= usable_cols) ? 1 : ((rlen + usable_cols - 1) / usable_cols);
            }
            int chunk_offset = (usable_cols > 0) ? (E.cx / usable_cols) : 0;
            cursor_y = w->top + cur_screen_y + chunk_offset + 1;
            cursor_x = w->left + 1 + num_w + ((usable_cols > 0) ? (E.cx % usable_cols) : 0);
        } else {
            int vis_y = 0;
            for (int r = E.row_off; r < E.cy && r < E.num_lines; r++) {
                if (!fold_is_line_hidden(r)) vis_y++;
            }
            cursor_y = w->top + vis_y + 1;
            cursor_x = w->left + (E.screen_x - E.col_off) + 1 + num_w;
        }
    }

    ab_printf(&ab, "\x1b[%d;%dH\x1b[?25h\x1b[?2026l", cursor_y, cursor_x);

    if (E.hOut && E.hOut != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(E.hOut, ab.b, ab.len, &written, NULL);
    } else {
        fwrite(ab.b, 1, ab.len, stdout);
        fflush(stdout);
    }
    ab_free(&ab);
}
