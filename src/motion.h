#ifndef MOTION_H
#define MOTION_H

#include "vi.h"

MotionResult motion_word_forward(int count, bool big_word);
MotionResult motion_word_backward(int count, bool big_word);
MotionResult motion_word_end(int count, bool big_word);
MotionResult motion_inline_find(char cmd, char target, int count);
MotionResult motion_bracket_match(void);
MotionResult evaluate_motion(int key, int count);
bool evaluate_text_object(char scope, char target, int *out_y1, int *out_x1, int *out_y2, int *out_x2);

#endif /* MOTION_H */
