#ifndef TERM_H
#define TERM_H

enum {
    KEY_ARROW_UP = 1001,
    KEY_ARROW_DOWN = 1002,
    KEY_ARROW_LEFT = 1003,
    KEY_ARROW_RIGHT = 1004,
    KEY_HOME = 1005,
    KEY_END = 1006,
    KEY_PAGE_UP = 1007,
    KEY_PAGE_DOWN = 1008,
    KEY_DEL = 1009,
    KEY_F1 = 1010,
    KEY_MOUSE_PRESS = 1020,
    KEY_MOUSE_RELEASE = 1021,
    KEY_MOUSE_WHEEL_UP = 1022,
    KEY_MOUSE_WHEEL_DOWN = 1023
};

void term_enable_raw(void);
void term_disable_raw(void);
void term_set_mouse(bool enable);
void term_get_window_size(void);
int term_read_key(void);
void term_push_key(int key);

#endif /* TERM_H */
