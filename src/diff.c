#include "vi.h"
#include "diff.h"
#include "buffer.h"
#include "window.h"
#include "screen.h"

static DiffStatus *s_buf_diff[MAX_BUFFERS] = {NULL};
static int s_buf_diff_len[MAX_BUFFERS] = {0};

void diff_init(void) {
    for (int i = 0; i < MAX_BUFFERS; i++) {
        s_buf_diff[i] = NULL;
        s_buf_diff_len[i] = 0;
    }
}

void diff_cleanup(void) {
    for (int i = 0; i < MAX_BUFFERS; i++) {
        if (s_buf_diff[i]) {
            free(s_buf_diff[i]);
            s_buf_diff[i] = NULL;
            s_buf_diff_len[i] = 0;
        }
    }
}

static unsigned int hash_line(const char *s, int len) {
    unsigned int h = 5381;
    for (int i = 0; i < len; i++) {
        h = ((h << 5) + h) + (unsigned char)s[i];
    }
    return h;
}

DiffStatus diff_get_status(int buf_idx, int line_idx) {
    if (buf_idx < 0 || buf_idx >= MAX_BUFFERS) return DIFF_NONE;
    if (!s_buf_diff[buf_idx] || line_idx < 0 || line_idx >= s_buf_diff_len[buf_idx]) {
        return DIFF_NONE;
    }
    return s_buf_diff[buf_idx][line_idx];
}

void diff_compute(void) {
    /* Find two windows with diff_mode */
    int winA = -1, winB = -1;
    for (int i = 0; i < E.num_windows; i++) {
        if (E.windows[i].diff_mode) {
            if (winA == -1) winA = i;
            else if (winB == -1) { winB = i; break; }
        }
    }

    diff_cleanup();

    if (winA == -1 || winB == -1) return;

    int bufA = E.windows[winA].buf_idx;
    int bufB = E.windows[winB].buf_idx;
    if (bufA < 0 || bufA >= E.num_buffers || bufB < 0 || bufB >= E.num_buffers) return;

    /* Get lines for buffer A */
    Line *linesA = (bufA == E.cur_buf_idx) ? E.lines : E.buffers[bufA].lines;
    int numA = (bufA == E.cur_buf_idx) ? E.num_lines : E.buffers[bufA].num_lines;

    /* Get lines for buffer B */
    Line *linesB = (bufB == E.cur_buf_idx) ? E.lines : E.buffers[bufB].lines;
    int numB = (bufB == E.cur_buf_idx) ? E.num_lines : E.buffers[bufB].num_lines;

    if (!linesA || !linesB || numA <= 0 || numB <= 0) return;

    s_buf_diff[bufA] = (DiffStatus *)calloc(numA, sizeof(DiffStatus));
    s_buf_diff_len[bufA] = numA;

    s_buf_diff[bufB] = (DiffStatus *)calloc(numB, sizeof(DiffStatus));
    s_buf_diff_len[bufB] = numB;

    /* For large files, cap LCS matrix */
    if ((long long)numA * (long long)numB > 4000000LL) {
        /* Line-by-line simple matching */
        int lim = min(numA, numB);
        for (int i = 0; i < lim; i++) {
            if (linesA[i].len == linesB[i].len && strcmp(linesA[i].chars, linesB[i].chars) == 0) {
                s_buf_diff[bufA][i] = DIFF_SAME;
                s_buf_diff[bufB][i] = DIFF_SAME;
            } else {
                s_buf_diff[bufA][i] = DIFF_CHANGE;
                s_buf_diff[bufB][i] = DIFF_CHANGE;
            }
        }
        for (int i = lim; i < numA; i++) s_buf_diff[bufA][i] = DIFF_DEL;
        for (int i = lim; i < numB; i++) s_buf_diff[bufB][i] = DIFF_ADD;
        return;
    }

    unsigned int *hA = (unsigned int *)malloc(numA * sizeof(unsigned int));
    unsigned int *hB = (unsigned int *)malloc(numB * sizeof(unsigned int));
    for (int i = 0; i < numA; i++) hA[i] = hash_line(linesA[i].chars, linesA[i].len);
    for (int i = 0; i < numB; i++) hB[i] = hash_line(linesB[i].chars, linesB[i].len);

    int stride = numB + 1;
    int *dp = (int *)calloc((numA + 1) * stride, sizeof(int));

    for (int i = 1; i <= numA; i++) {
        for (int j = 1; j <= numB; j++) {
            if (hA[i - 1] == hB[j - 1] && strcmp(linesA[i - 1].chars, linesB[j - 1].chars) == 0) {
                dp[i * stride + j] = dp[(i - 1) * stride + (j - 1)] + 1;
            } else {
                int v1 = dp[(i - 1) * stride + j];
                int v2 = dp[i * stride + (j - 1)];
                dp[i * stride + j] = (v1 > v2) ? v1 : v2;
            }
        }
    }

    /* Backtrack LCS */
    int i = numA, j = numB;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && hA[i - 1] == hB[j - 1] &&
            strcmp(linesA[i - 1].chars, linesB[j - 1].chars) == 0) {
            s_buf_diff[bufA][i - 1] = DIFF_SAME;
            s_buf_diff[bufB][j - 1] = DIFF_SAME;
            i--;
            j--;
        } else if (j > 0 && (i == 0 || dp[i * stride + (j - 1)] >= dp[(i - 1) * stride + j])) {
            s_buf_diff[bufB][j - 1] = DIFF_ADD;
            j--;
        } else if (i > 0) {
            s_buf_diff[bufA][i - 1] = DIFF_DEL;
            i--;
        }
    }

    /* Identify modified lines: adjacent ADD/DEL blocks are considered CHANGES */
    for (int k = 0; k < numA; k++) {
        if (s_buf_diff[bufA][k] == DIFF_DEL && k < numB && s_buf_diff[bufB][k] == DIFF_ADD) {
            s_buf_diff[bufA][k] = DIFF_CHANGE;
            s_buf_diff[bufB][k] = DIFF_CHANGE;
        }
    }

    free(hA);
    free(hB);
    free(dp);
}

void diff_sync_scroll(void) {
    if (E.cur_win_idx < 0 || E.cur_win_idx >= E.num_windows) return;
    EditorWindow *cur = &E.windows[E.cur_win_idx];
    if (!cur->scrollbind) return;

    for (int i = 0; i < E.num_windows; i++) {
        if (i != E.cur_win_idx && E.windows[i].scrollbind) {
            E.windows[i].row_off = cur->row_off;
        }
    }
}

bool diff_split(const char *filename) {
    if (!filename || *filename == '\0') return false;

    /* Current window enables diff */
    if (E.cur_win_idx >= 0 && E.cur_win_idx < E.num_windows) {
        E.windows[E.cur_win_idx].diff_mode = true;
        E.windows[E.cur_win_idx].scrollbind = true;
    }

    /* Split vertically */
    if (!window_split(true)) return false;

    /* Open target file in new split */
    editor_open(filename);

    if (E.cur_win_idx >= 0 && E.cur_win_idx < E.num_windows) {
        E.windows[E.cur_win_idx].diff_mode = true;
        E.windows[E.cur_win_idx].scrollbind = true;
    }

    diff_compute();
    set_status("Diff mode enabled with \"%s\"", filename);
    return true;
}

void diff_this(void) {
    if (E.cur_win_idx < 0 || E.cur_win_idx >= E.num_windows) return;
    E.windows[E.cur_win_idx].diff_mode = true;
    E.windows[E.cur_win_idx].scrollbind = true;
    diff_compute();
    set_status("Diff mode enabled on window %d", E.windows[E.cur_win_idx].id);
}

void diff_off(void) {
    for (int i = 0; i < E.num_windows; i++) {
        E.windows[i].diff_mode = false;
        E.windows[i].scrollbind = false;
    }
    diff_cleanup();
    set_status("Diff mode disabled");
}
