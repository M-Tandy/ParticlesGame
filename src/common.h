#ifndef ptest_common_h
#define ptest_common_h

#include "raylib.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #define DEBUG_QUADINFO
// #define DEBUG_DRAW_QUADS

// #define DEBUG_CELL_INFO

#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)
#define CONSTRAIN(x, min, max) (MIN(MAX(x, min), max))

#define IN_BBOX(vector, bbox) (vector.x >= bbox.min.x && vector.y >= bbox.min.y && vector.x <= bbox.max.x && vector.y <= bbox.max.y)
#define IN_RECT(vector, rect) (vector.x >= rect.x && vector.x <= rect.x + rect.width && vector.y >= rect.y && vector.y <= rect.y + rect.height)
#define IN_SQUARE(vector, center, width)                                                                                                                                                               \
    ((vector).x >= (center).x - width / 2.0f && vector.x <= center.x + width / 2.0f && (vector).y >= (center).y - width / 2.0f && vector.y <= center.y + width / 2.0f)
#define SWAP(x, y, T)                                                                                                                                                                                  \
    do {                                                                                                                                                                                               \
        T SWAP = x;                                                                                                                                                                                    \
        x = y;                                                                                                                                                                                         \
        y = SWAP;                                                                                                                                                                                      \
    } while (0)

#define MAP(count, A, mapping)                                                                                                                                                                         \
    for (int I = 0; I < count; I++) {                                                                                                                                                                  \
        A[I] = mapping(A[I])                                                                                                                                                                           \
    }

#define MAP2D(rows, cols, A, mapping)                                                                                                                                                                  \
    for (int I = 0; I < rows; I++) {                                                                                                                                                                   \
        for (int J = 0; J < cols; J++) {                                                                                                                                                               \
            A[I][J] = mapping(A[I][J])                                                                                                                                                                 \
        }                                                                                                                                                                                              \
    }

#define ORIGIN (Vector2){0.0f, 0.0f}
#define WIDTH 1024
#define HEIGHT 1024
#define CELLPOWER 5
#define GRIDWIDTH 2048.0f / 2
#define UPDATE_RATE 60
#define FLUID_AMOUNT 64

#define CAMERA_SPEED 8

#define GRID_WIDTH 32
#define GRID_RENDER_WIDTH 2048
#define GRID_RENDER_HEIGHT 2048

typedef struct Scene Scene;
Scene *scenePtr;

#endif // ptest_common_h
