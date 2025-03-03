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
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           1,    1)
else if CHAR('1',
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        1,    1,
           0,    0)
else if CHAR('2',
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           1,    1,
        0,    0,
           0,    0)
else if CHAR('3',
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        1,    1,
           0,    0,
        0,    0,
           0,    0)
else if CHAR('4',
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           1,    1,
        0,    0,
           0,    0,
        0,    0,
           0,    0)
else if CHAR('5',
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        1,    1,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0)
else if CHAR('6',
        0,    0,
           0,    0,
        0,    0,
           1,    1,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0)
else if CHAR('7',
        0,    0,
           0,    0,
        1,    1,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0)
else if CHAR('8',
        0,    0,
           1,    1,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0)
else if CHAR('9',
        1,    1,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0,
        0,    0,
           0,    0)
else if (character == ':') {
   Set(40, beginColor);
   Set(41, endColor);
   return 2;
} else if (character == '.') {
   Set(40, beginColor);
   return 2;
} else if (character == CHAR_UP_ARROW) {
   Set(91, beginColor);
   return 2;
} else if (character == CHAR_DOWN_ARROW) {
   Set(90, beginColor);
   return 2;
}