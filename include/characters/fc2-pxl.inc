// This file implements characters as lines of code within pixels.hpp
// It is not meant to be included elsewhere.

// clang-format off
#define CHAR(c, ...) (character == c) { charData = { __VA_ARGS__ }; }
if CHAR(' ',
          0, 0, 0, 0,
          0, 0, 0, 0,
          0, 0, 0, 0,
          0, 0, 0, 0,
          0, 0, 0, 0)
else if CHAR('0',
        0,    1,
           1,    1,
        1,    0,
           0,    1,
        1,    0,
           0,    1,
        1,    0,
           0,    1,
        1,    1,
           1,    0)
else if CHAR('1',
        0,    1,
           1,    0,
        0,    1,
           0,    0,
        0,    1,
           0,    0,
        0,    1,
           0,    0,
        0,    1,
           0,    0)
else if CHAR('2',
        0,    1,
           1,    1,
        1,    0,
           0,    1,
        0,    1,
           1,    0,
        1,    0,
           0,    0,
        1,    1,
           1,    1)
else if CHAR('3',
        1,    1,
           1,    1,
        0,    0,
           0,    1,
        0,    1,
           0,    1,
        0,    0,
           0,    1,
        1,    1,
           1,    0)
else if CHAR('4',
        0,    0,
           0,    1,
        0,    1,
           1,    1,
        1,    0,
           1,    1,
        1,    1,
           0,    1,
        0,    0,
           0,    1)
else if CHAR('5',
        1,    1,
           1,    1,
        1,    0,
           1,    0,
        0,    1,
           0,    1,
        0,    0,
           0,    1,
        1,    1,
           1,    0)
else if CHAR('6',
        0,    1,
           1,    0,
        1,    0,
           0,    0,
        1,    1,
           1,    1,
        1,    0,
           0,    1,
        1,    1,
           1,    0)
else if CHAR('7',
        1,    1,
           1,    1,
        0,    0,
           0,    1,
        0,    1,
           0,    0,
        0,    1,
           1,    0,
        0,    0,
           1,    0)
else if CHAR('8',
        0,    1,
           1,    1,
        1,    0,
           0,    1,
        1,    1,
           1,    1,
        1,    0,
           0,    1,
        1,    1,
           1,    0)
else if CHAR('9',
        0,    1,
           1,    1,
        1,    0,
           0,    1,
        1,    1,
           1,    1,
        0,    0,
           0,    1,
        0,    1,
           1,    0)
else if CHAR(CHAR_RIGHT_ARROW,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    1,
           0,    1,
        0,    1,
           0,    0,
        0,    0,
           0,    0)
else if CHAR(CHAR_LEFT_ARROW,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    1,
           1,    0,
        0,    1,
           0,    0,
        0,    0,
           0,    0)
else if (character == ':') {
   Set(40, color);
   Set(41, color);
   return 2;
} else if (character == '.') {
   Set(40, color);
   return 2;
} else if (character == CHAR_UP_ARROW) {
   Set(91, color);
   return 2;
} else if (character == CHAR_DOWN_ARROW) {
   Set(90, color);
   return 2;
}