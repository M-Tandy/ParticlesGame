#ifndef ptest_common_h
#define ptest_common_h

#include "raylib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #define DEBUG_QUADINFO
#define DEBUG_DRAW_QUADS

#define IN_BBOX(vector, bbox)                                                                                          \
    (vector.x >= bbox.min.x && vector.y >= bbox.min.y && vector.x <= bbox.max.x && vector.y <= bbox.max.y)
#define IN_RECT(vector, rect)                                                                                          \
    (vector.x >= rect.x && vector.x <= rect.x + rect.width && vector.y >= rect.y && vector.y <= rect.y + rect.height)
#define IN_SQUARE(vector, center, width)                                                                               \
    (vector.x >= center.x - width / 2.0f && vector.x <= center.x + width / 2.0f &&                                     \
     vector.y >= center.y - width / 2.0f && vector.y <= center.y + width / 2.0f)

#endif // ptest_common_h
