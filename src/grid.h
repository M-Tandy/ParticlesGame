#ifndef ptest_grid_h
#define ptest_grid_h

#include "value.h"
#include "data.h"
#include <stdint.h>

typedef struct Grid {
    Cell **cells;
    int rows;
    int cols;
} Grid;

Scene GridScene;

#endif // ptest_grid_h
