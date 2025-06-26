#ifndef ptest_common_h
#define ptest_common_h

#include "raylib.h"

#define IN_BBOX(vector, bbox)                                                                                          \
    (vector.x >= bbox.min.x && vector.y >= bbox.min.y && vector.x <= bbox.max.x && vector.y <= bbox.max.y)
#define IN_RECT(vector, rect)                                                                                          \
    (vector.x >= rect.x && vector.x <= rect.x + rect.width && vector.y >= rect.y && vector.y <= rect.y + rect.height)

#endif // ptest_common_h
