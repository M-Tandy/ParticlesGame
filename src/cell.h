#ifndef ptest_cell_h
#define ptest_cell_h

#include <stdbool.h>
#include <stdint.h>
#include "raylib.h"
#include "raymath.h"

typedef struct Cell {
    bool state;
} Cell;

typedef struct Grid {
    Cell **cells;
    uint8_t rows;
    uint8_t cols;

    char *label;
} Grid;

void initCell(Cell *cell);

void initGrid(Grid *grid, uint8_t rows, uint8_t cols, char* label);
int gridDrawWidth(int scale, Grid grid);
int gridDrawHeight(int scale, Grid grid);
void drawGrid(int x, int y, int scale, int cellPadding, Grid grid);
void freeGrid(Grid *grid);
void updateGrids(Grid *grid1, Grid *grid2);
BoundingBox gridBoundingBox(int x, int y, int scale, Grid grid);
Vector2 positionToGridLocation(float x, float y, int gridx, int gridy, int scale);
Cell *positionToGridCell(int x, int y, int gridx, int gridy, int scale, const Grid *grid);

#endif // ptest_cell_h
