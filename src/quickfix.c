#include "vi.h"
#include "quickfix.h"
#include "buffer.h"
#include "window.h"
#include "screen.h"
#include "term.h"

typedef struct {
    HANDLE hThread;
    char pattern[256];
    volatile bool running;
    volatile bool cancel;
    volatile int items_found;
    CRITICAL_SECTION cs;
    bool cs_init;
} AsyncGrepState;

static AsyncGrepState s_async_grep = {0};

void quickfix_init(void) {
    memset(&E.qf, 0, sizeof(QuickfixState));
    E.qf.cur_idx = -1;
    if (!s_async_grep.cs_init) {
        InitializeCriticalSection(&s_async_grep.cs);
        s_async_grep.cs_init = true;
    }
}

void quickfix_clear(void) {
    if (s_async_grep.cs_init) EnterCriticalSection(&s_async_grep.cs);
    E.qf.count = 0;
    E.qf.cur_idx = -1;
    E.qf.is_open = false;
    if (s_async_grep.cs_init) LeaveCriticalSection(&s_async_grep.cs);
}

void quickfix_add(const char *filename, int line, int col, const char *text) {
    if (!filename) return;
    if (s_async_grep.cs_init) EnterCriticalSection(&s_async_grep.cs);
    if (E.qf.count < MAX_QUICKFIX) {
        QuickfixEntry *qe = &E.qf.entries[E.qf.count++];
        strncpy(qe->filename, filename, sizeof(qe->filename) - 1);
        qe->filename[sizeof(qe->filename) - 1] = '\0';
        qe->line_num = line;
        qe->col_num = col;
        if (text) {
            strncpy(qe->text, text, sizeof(qe->text) - 1);
            qe->text[sizeof(qe->text) - 1] = '\0';
        } else {
            qe->text[0] = '\0';
        }
    }
    if (s_async_grep.cs_init) LeaveCriticalSection(&s_async_grep.cs);
}

static DWORD WINAPI grep_worker_thread(LPVOID param) {
    (void)param;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "findstr /s /n /i /c:\"%s\" *.* 2>nul", s_async_grep.pattern);

    FILE *pipe = _popen(cmd, "r");
    if (!pipe) {
        s_async_grep.running = false;
        return 0;
    }

    char line_buf[2048];
    while (!s_async_grep.cancel && fgets(line_buf, sizeof(line_buf), pipe)) {
        int len = (int)strlen(line_buf);
        while (len > 0 && (line_buf[len - 1] == '\r' || line_buf[len - 1] == '\n')) {
            line_buf[--len] = '\0';
        }
        if (len == 0) continue;

        if (strstr(line_buf, ".swp") || strstr(line_buf, ".git\\") || strstr(line_buf, ".git/")) {
            continue;
        }

        char *p = line_buf;
        if (isalpha((unsigned char)p[0]) && p[1] == ':') {
            p += 2;
        }
        char *first_colon = strchr(p, ':');
        if (!first_colon) continue;

        char *second_colon = strchr(first_colon + 1, ':');
        if (!second_colon) continue;

        *first_colon = '\0';
        *second_colon = '\0';

        const char *fname = line_buf;
        int lineno = atoi(first_colon + 1);
        const char *match_text = second_colon + 1;
        while (*match_text == ' ' || *match_text == '\t') match_text++;

        quickfix_add(fname, lineno, 0, match_text);
        s_async_grep.items_found++;
    }
    _pclose(pipe);

    s_async_grep.running = false;
    return 0;
}

bool quickfix_is_running(void) {
    return s_async_grep.running;
}

void quickfix_cancel(void) {
    if (s_async_grep.running) {
        s_async_grep.cancel = true;
        if (s_async_grep.hThread) {
            WaitForSingleObject(s_async_grep.hThread, 500);
            CloseHandle(s_async_grep.hThread);
            s_async_grep.hThread = NULL;
        }
        s_async_grep.running = false;
    }
}

void quickfix_wait(DWORD timeout_ms) {
    if (s_async_grep.hThread) {
        WaitForSingleObject(s_async_grep.hThread, timeout_ms);
        CloseHandle(s_async_grep.hThread);
        s_async_grep.hThread = NULL;
        s_async_grep.running = false;
    }
}

int quickfix_get_async_progress(char *out_pattern, int max_pat) {
    if (out_pattern && max_pat > 0) {
        strncpy(out_pattern, s_async_grep.pattern, max_pat - 1);
        out_pattern[max_pat - 1] = '\0';
    }
    return s_async_grep.items_found;
}

bool quickfix_check_completed(void) {
    static bool was_running = false;
    if (was_running && !s_async_grep.running) {
        was_running = false;
        if (s_async_grep.hThread) {
            CloseHandle(s_async_grep.hThread);
            s_async_grep.hThread = NULL;
        }
        set_status("Grep complete: %d matches found for \"%s\"", E.qf.count, s_async_grep.pattern);
        if (E.qf.count > 0 && E.qf.cur_idx < 0) {
            quickfix_jump(0);
        }
        return true;
    }
    was_running = s_async_grep.running;
    return false;
}

bool quickfix_run_grep_async(const char *pattern) {
    if (!pattern || *pattern == '\0') {
        set_status("E471: Argument required (:grep <pattern>)");
        return false;
    }

    quickfix_cancel();
    quickfix_clear();

    strncpy(s_async_grep.pattern, pattern, sizeof(s_async_grep.pattern) - 1);
    s_async_grep.pattern[sizeof(s_async_grep.pattern) - 1] = '\0';
    s_async_grep.running = true;
    s_async_grep.cancel = false;
    s_async_grep.items_found = 0;

    s_async_grep.hThread = CreateThread(NULL, 0, grep_worker_thread, NULL, 0, NULL);
    if (!s_async_grep.hThread) {
        s_async_grep.running = false;
        set_status("Failed to create background search thread");
        return false;
    }

    set_status("Searching for \"%s\" in background... (use :copen to view)", pattern);
    return true;
}

bool quickfix_run_grep(const char *pattern) {
    bool ok = quickfix_run_grep_async(pattern);
    return ok;
}

bool quickfix_jump(int index) {
    if (index < 0 || index >= E.qf.count) return false;
    E.qf.cur_idx = index;
    QuickfixEntry *qe = &E.qf.entries[index];

    if (E.filename && strcmp(E.filename, "[Quickfix List]") == 0) {
        if (E.num_windows > 1) {
            window_prev();
        }
    }

    editor_open(qe->filename);
    if (qe->line_num > 0) {
        E.cy = min(E.num_lines - 1, max(0, qe->line_num - 1));
        E.cx = 0;
    }
    set_status("(%d of %d): %s:%d: %s",
               index + 1, E.qf.count, qe->filename, qe->line_num, qe->text);
    return true;
}

bool quickfix_next(void) {
    if (E.qf.count == 0) {
        set_status("E42: No Errors");
        return false;
    }
    if (E.qf.cur_idx + 1 >= E.qf.count) {
        set_status("E553: No more items");
        return false;
    }
    return quickfix_jump(E.qf.cur_idx + 1);
}

bool quickfix_prev(void) {
    if (E.qf.count == 0) {
        set_status("E42: No Errors");
        return false;
    }
    if (E.qf.cur_idx <= 0) {
        set_status("E553: No more items");
        return false;
    }
    return quickfix_jump(E.qf.cur_idx - 1);
}

void quickfix_open_window(int height) {
    if (E.qf.count == 0 && !s_async_grep.running) {
        set_status("E42: No Errors");
        return;
    }

    for (int i = 0; i < E.num_buffers; i++) {
        if (E.buffers[i].filename && strcmp(E.buffers[i].filename, "[Quickfix List]") == 0) {
            buffer_switch_to(i);
            E.qf.is_open = true;
            return;
        }
    }

    if (height <= 0) height = 8;
    window_split(false);
    E.windows[E.cur_win_idx].rows = min(height, E.screen_rows / 2);

    buffer_add("[Quickfix List]");
    buffer_switch_by_name("[Quickfix List]");

    while (E.num_lines > 0) {
        delete_row(0);
    }

    if (s_async_grep.cs_init) EnterCriticalSection(&s_async_grep.cs);
    for (int i = 0; i < E.qf.count; i++) {
        char line[512];
        snprintf(line, sizeof(line), "%s:%d: %s",
                 E.qf.entries[i].filename,
                 E.qf.entries[i].line_num,
                 E.qf.entries[i].text);
        insert_row(i, line, (int)strlen(line));
    }
    if (s_async_grep.cs_init) LeaveCriticalSection(&s_async_grep.cs);

    E.readonly = true;
    E.dirty = false;
    E.qf.is_open = true;
    set_status("Quickfix (%d items%s)", E.qf.count, s_async_grep.running ? " [searching...]" : "");
}

void quickfix_close_window(void) {
    if (E.filename && strcmp(E.filename, "[Quickfix List]") == 0) {
        window_close_current();
    }
    E.qf.is_open = false;
}

void quickfix_list(void) {
    if (E.qf.count == 0) {
        set_status("E42: No Errors");
        return;
    }
    printf("\x1b[2J\x1b[H=== Quickfix List (%d items%s) ===\r\n",
           E.qf.count, s_async_grep.running ? " [searching...]" : "");
    int show_count = min(E.qf.count, 20);
    for (int i = 0; i < show_count; i++) {
        printf("%s%2d: %s:%d: %s\r\n",
               (i == E.qf.cur_idx) ? ">" : " ",
               i + 1,
               E.qf.entries[i].filename,
               E.qf.entries[i].line_num,
               E.qf.entries[i].text);
    }
    if (E.qf.count > 20) {
        printf("... (%d more matches)\r\n", E.qf.count - 20);
    }
    printf("\r\n[Hit return to continue]");
    term_read_key();
}

bool quickfix_handle_enter(void) {
    if (!E.filename || strcmp(E.filename, "[Quickfix List]") != 0) {
        return false;
    }
    int selected = E.cy;
    if (selected >= 0 && selected < E.qf.count) {
        quickfix_jump(selected);
        return true;
    }
    return false;
}
