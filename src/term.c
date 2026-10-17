#include "vi.h"
#include "term.h"

#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif

void term_set_mouse(bool enable) {
    E.opt.mouse = enable;
    if (enable) {
        printf("\x1b[?1000h\x1b[?1002h\x1b[?1006h");
    } else {
        printf("\x1b[?1006l\x1b[?1002l\x1b[?1000l");
    }
    fflush(stdout);
}

void term_enable_raw(void) {
    E.hIn = GetStdHandle(STD_INPUT_HANDLE);
    E.hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleMode(E.hIn, &E.orig_in_mode);
    GetConsoleMode(E.hOut, &E.orig_out_mode);

    DWORD in_mode = E.orig_in_mode;
    in_mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE);
    in_mode |= ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT;
    SetConsoleMode(E.hIn, in_mode);

    DWORD out_mode = E.orig_out_mode;
    out_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
    SetConsoleMode(E.hOut, out_mode);

    if (E.opt.mouse) {
        printf("\x1b[?1000h\x1b[?1002h\x1b[?1006h");
        fflush(stdout);
    }
}

void term_disable_raw(void) {
    printf("\x1b[?1006l\x1b[?1002l\x1b[?1000l\x1b[?25h\x1b[0m\x1b[2J\x1b[H");
    fflush(stdout);
    SetConsoleMode(E.hIn, E.orig_in_mode);
    SetConsoleMode(E.hOut, E.orig_out_mode);
}

void term_get_window_size(void) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(E.hOut, &csbi)) {
        E.screen_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int total_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (E.opt.ruler) {
            E.screen_rows = total_rows - 2; /* 1 row for status bar, 1 for cmd line */
        } else {
            E.screen_rows = total_rows - 1; /* 1 row for cmd / message line */
        }
    } else {
        E.screen_cols = 80;
        E.screen_rows = 24;
    }
    if (E.screen_rows < 1) E.screen_rows = 1;
    if (E.screen_cols < 1) E.screen_cols = 1;
}

void term_push_key(int key) {
    int next = (E.macro_queue.head + 1) % MACRO_BUF_SIZE;
    if (next != E.macro_queue.tail) {
        E.macro_queue.keys[E.macro_queue.head] = key;
        E.macro_queue.head = next;
    }
}

int term_read_key(void) {
    if (E.macro_queue.head != E.macro_queue.tail) {
        int key = E.macro_queue.keys[E.macro_queue.tail];
        E.macro_queue.tail = (E.macro_queue.tail + 1) % MACRO_BUF_SIZE;
        return key;
    }

    int c = _getch();
    if (c == 0 || c == 0xE0) {
        int ext = _getch();
        switch (ext) {
            case 72: return KEY_ARROW_UP;
            case 80: return KEY_ARROW_DOWN;
            case 75: return KEY_ARROW_LEFT;
            case 77: return KEY_ARROW_RIGHT;
            case 71: return KEY_HOME;
            case 79: return KEY_END;
            case 73: return KEY_PAGE_UP;
            case 81: return KEY_PAGE_DOWN;
            case 83: return KEY_DEL;
            case 59: return KEY_F1;
            default: return 0;
        }
    } else if (c == 27) {
        if (_kbhit()) {
            int seq1 = _getch();
            if (seq1 == '[' || seq1 == 'O') {
                if (_kbhit()) {
                    int seq2 = _getch();
                    if (seq1 == '[') {
                        if (seq2 == '<') {
                            char buf[32];
                            int bidx = 0;
                            char end_ch = 0;
                            while (bidx < (int)sizeof(buf) - 1) {
                                int ch = _getch();
                                if (ch == 'M' || ch == 'm') {
                                    end_ch = (char)ch;
                                    buf[bidx] = '\0';
                                    break;
                                }
                                buf[bidx++] = (char)ch;
                            }
                            if (end_ch) {
                                int b = 0, x = 0, y = 0;
                                if (sscanf(buf, "%d;%d;%d", &b, &x, &y) == 3) {
                                    E.mouse_b = b;
                                    E.mouse_x = x;
                                    E.mouse_y = y;
                                    E.mouse_release = (end_ch == 'm');
                                    if (b == 64) {
                                        return KEY_MOUSE_WHEEL_UP;
                                    } else if (b == 65) {
                                        return KEY_MOUSE_WHEEL_DOWN;
                                    } else if (!E.mouse_release) {
                                        return KEY_MOUSE_PRESS;
                                    } else {
                                        return KEY_MOUSE_RELEASE;
                                    }
                                }
                            }
                            return 0;
                        } else if (seq2 >= '0' && seq2 <= '9') {
                            if (_kbhit()) {
                                int seq3 = _getch();
                                if (seq3 == '~') {
                                    switch (seq2) {
                                        case '1': return KEY_HOME;
                                        case '3': return KEY_DEL;
                                        case '4': return KEY_END;
                                        case '5': return KEY_PAGE_UP;
                                        case '6': return KEY_PAGE_DOWN;
                                        case '7': return KEY_HOME;
                                        case '8': return KEY_END;
                                    }
                                }
                            }
                        } else {
                            switch (seq2) {
                                case 'A': return KEY_ARROW_UP;
                                case 'B': return KEY_ARROW_DOWN;
                                case 'C': return KEY_ARROW_RIGHT;
                                case 'D': return KEY_ARROW_LEFT;
                                case 'H': return KEY_HOME;
                                case 'F': return KEY_END;
                            }
                        }
                    } else if (seq1 == 'O') {
                        switch (seq2) {
                            case 'A': return KEY_ARROW_UP;
                            case 'B': return KEY_ARROW_DOWN;
                            case 'C': return KEY_ARROW_RIGHT;
                            case 'D': return KEY_ARROW_LEFT;
                            case 'H': return KEY_HOME;
                            case 'F': return KEY_END;
                            case 'P': return KEY_F1;
                        }
                    }
                }
            }
        }
        return 27;
    }
    return c;
}
