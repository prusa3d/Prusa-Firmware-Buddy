///
/// Library of flexible functions not defined in standard C99
///

#include <stdlib.h>

#ifndef CMATH_EXT

    #define MAX(a, b) \
        ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a >= _b ? _a : _b; })

    #define MIN(a, b) \
        ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a <= _b ? _a : _b; })

    /// -1 = negative, 1 = positive, 0 = 0
    #define SIGN0(x) \
        ({ __typeof__ (x) _x = (x); \
    (_x > 0) - (_x < 0); })

    /// -1 = negative, 1 = positive or 0
    #define SIGN1(x) \
        ({ __typeof__ (x) _x = (x); \
    (_x >= 0) - (_x < 0); })

    /// \returns positive value always
    #define ABS(a) \
        ({ __typeof__ (a) _a = (a); \
    _a >= 0 ? _a : (-_a); })

    /// swap values
    #define SWAP(a, b) \
        ({ __typeof__ (a) c = (a); \
        a = (b); \
        b = c; })

    /// \returns random integer in range <0,max_val>
    #define RAND(max_val) ((int)((float)rand() / RAND_MAX * (max_val)))

    /// \returns random float in range <0,MAX>
    #define FRAND(max_val) ((float)rand() / RAND_MAX * (max_val));

#endif
