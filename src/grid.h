#ifndef ptest_grid_h
#define ptest_grid_h

#include "value.h"
#include <stdint.h>

typedef struct Grid {
    CellValue **cells;
    int rows;
    int cols;
} Grid;

void initGrid(Grid *grid, uint16_t rows, uint16_t cols);
void freeGrid(Grid *grid);

void drawGridPixels(const Grid *grid, int x, int y);

int gridDrawWidth(int scale, Grid grid);
int gridDrawHeight(int scale, Grid grid);
void drawGrid(const Grid *grid, int x, int y, int cellWidth, int cellHeight, int spacing);

bool getCellAt(const Grid *grid, int grid_x, int grid_y, float x, float y, int cellWidth, int cellHeight, CellValue **result);
void evolveGrid(const Grid *grid, Grid *result);

#endif // ptest_grid_h
