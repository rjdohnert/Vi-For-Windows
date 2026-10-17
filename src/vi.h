#ifndef VI_H
#define VI_H

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define VI_VERSION "Vi 5.0.0-2026"

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* --- Vi Modes --- */
typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_REPLACE,
    MODE_VISUAL,
    MODE_VISUAL_LINE,
    MODE_VISUAL_BLOCK,
    MODE_COMMAND,
    MODE_SEARCH,
    MODE_HELP,
    MODE_EX
} EditorMode;

/* --- Register Types --- */
typedef enum {
    VI_REG_NONE,
    VI_REG_CHAR,
    VI_REG_LINE,
    VI_REG_BLOCK
} RegType;

typedef struct {
    char *data;
    int len;
    RegType type;
} Register;

/* --- Line Structure --- */
typedef struct {
    char *chars;
    int len;
    int cap;
} Line;

/* --- Multi-Level Undo / Redo --- */
#define MAX_UNDO_LEVELS 100
typedef struct {
    Line *lines;
    int num_lines;
    int cx, cy;
} UndoSnapshot;

typedef struct {
    UndoSnapshot items[MAX_UNDO_LEVELS];
    int top;
} UndoStack;

/* --- Mark Position --- */
typedef struct {
    int y, x;
    bool set;
} Mark;

/* --- Motion Evaluation Result --- */
typedef struct {
    int y, x;
    bool linewise;
    bool inclusive;
    bool valid;
} MotionResult;

/* --- Dot (.) Repeat Record --- */
typedef enum {
    ACT_NONE,
    ACT_DIRECT,
    ACT_INSERT,
    ACT_CHANGE_MOTION,
    ACT_DELETE_MOTION
} ActionType;

typedef struct {
    ActionType type;
    char op;
    int count;
    char motion;
    int motion_count;
    char arg_char;
    char *inserted;
    int inserted_len;
    int inserted_cap;
    bool valid;
} RepeatRecord;

/* --- History Ring --- */
#define MAX_HIST 32
typedef struct {
    char items[MAX_HIST][256];
    int count;
} History;

/* --- Macro Key Queue --- */
#define MACRO_BUF_SIZE 4096
typedef struct {
    int keys[MACRO_BUF_SIZE];
    int head, tail;
} KeyQueue;

/* --- In-Memory Buffer --- */
typedef struct {
    int id;
    char *filename;
    Line *lines;
    int num_lines;
    bool dirty;
    bool is_crlf;
    bool readonly;
    int cx, cy;
    int screen_x;
    int row_off, col_off;
    UndoStack undo_stack;
    UndoStack redo_stack;
    char *u_line_backup;
    int u_line_cy;
    Mark marks[26];
    char swap_filename[MAX_PATH];
    bool loaded;
    FILETIME mod_time;
} Buffer;

#define MAX_BUFFERS 64

/* --- Window Viewport (Split Windows) --- */
#define MAX_WINDOWS 8
typedef struct {
    int id;
    int buf_idx;
    int top, left, rows, cols;
    int cx, cy;
    int screen_x;
    int row_off, col_off;
    bool diff_mode;
    bool scrollbind;
} EditorWindow;

/* --- Tab Pages --- */
#define MAX_TABS 16
typedef struct {
    int id;
    char title[64];
    EditorWindow windows[MAX_WINDOWS];
    int num_windows;
    int cur_win_idx;
} EditorTab;

/* --- Indent Code Folding --- */
#define MAX_FOLDS 128
typedef struct {
    int start_y;
    int end_y;
    int indent;
    bool collapsed;
} Fold;

/* --- Quickfix List --- */
typedef struct {
    char filename[MAX_PATH];
    int line_num;
    int col_num;
    char text[256];
} QuickfixEntry;

#define MAX_QUICKFIX 1024
typedef struct {
    QuickfixEntry entries[MAX_QUICKFIX];
    int count;
    int cur_idx;
    bool is_open;
} QuickfixState;

/* --- CTags Stack --- */
#define MAX_TAG_STACK 32
typedef struct {
    char filename[MAX_PATH];
    int cy, cx;
} TagStackEntry;

/* --- Buffer-Word Autocompletion --- */
#define MAX_COMPLETIONS 64
typedef struct {
    char items[MAX_COMPLETIONS][128];
    int count;
    int current_index;
    int prefix_len;
    int start_col;
    bool active;
} CompletionState;

/* --- Editor Options --- */
typedef struct {
    bool number;
    bool ignorecase;
    bool showmode;
    bool ruler;
    bool autoindent;
    bool hlsearch;
    bool nohl_active;
    bool syntax;
    bool compatible;
    bool incsearch;
    bool wrap;
    bool readonly;
    bool autoread;
    bool undofile;
    bool mouse;
    bool list;
    char colorscheme[32];
    char foldmethod[32];
    int tabstop;
    int shiftwidth;
} EditorOptions;

/* --- Editor Global State --- */
typedef struct {
    int cx, cy;
    int screen_x;
    int row_off, col_off;
    int screen_rows;
    int screen_cols;
    int mouse_x, mouse_y, mouse_b;
    bool mouse_release;
    int num_lines;
    Line *lines;
    char *filename;
    bool dirty;
    bool is_crlf;
    bool readonly;
    EditorMode mode;

    int visual_anchor_y, visual_anchor_x;

    /* Multi-Window Viewports */
    EditorWindow windows[MAX_WINDOWS];
    int num_windows;
    int cur_win_idx;
    int next_win_id;

    /* Tab Pages */
    EditorTab tabs[MAX_TABS];
    int num_tabs;
    int cur_tab_idx;
    int next_tab_id;

    /* Code Folds */
    Fold folds[MAX_FOLDS];
    int num_folds;

    /* Quickfix Engine */
    QuickfixState qf;

    /* Tag Stack for CTags */
    TagStackEntry tag_stack[MAX_TAG_STACK];
    int tag_stack_top;

    /* Multi-Buffer Table */
    Buffer buffers[MAX_BUFFERS];
    int num_buffers;
    int cur_buf_idx;
    int next_buffer_id;

    /* Autocompletion */
    CompletionState comp;
    char swap_filename[MAX_PATH];

    EditorOptions opt;

    char cmd_buf[512];
    int cmd_len;
    char status_msg[256];
    DWORD status_msg_time;

    /* History */
    History cmd_hist;
    History search_hist;
    int hist_idx;

    /* Multiple files argument list */
    char **args_files;
    int num_args;
    int cur_arg;

    /* Macros */
    bool recording_macro;
    char macro_reg;
    char last_macro_reg;
    KeyQueue macro_queue;

    char last_search[128];
    bool search_forward;
    char inline_char;
    char inline_cmd;

    Register reg_unnamed;
    Register reg_named[26];
    Register reg_numbered[9];
    Register reg_clipboard;
    char active_reg;

    Mark marks[26];
    Mark mark_prev;

    UndoStack undo_stack;
    UndoStack redo_stack;
    char *u_line_backup;
    int u_line_cy;

    int count1;
    int count2;
    char pending_op;
    bool reading_count2;

    RepeatRecord last_action;
    RepeatRecord curr_action;

    HANDLE hIn, hOut;
    DWORD orig_in_mode, orig_out_mode;

    int help_scroll;
} Editor;

extern Editor E;

#endif /* VI_H */
