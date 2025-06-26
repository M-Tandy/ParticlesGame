#ifndef ptest_common_h
#define ptest_common_h

#include "raylib.h"

#define IN_BBOX(vector, bbox)                                                                                          \
    (vector.x >= bbox.min.x && vector.y >= bbox.min.y && vector.x <= bbox.max.x && vector.y <= bbox.max.y)

#endif // ptest_common_h
