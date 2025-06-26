#ifndef ptest_cell_h
#define ptest_cell_h

#include <stdbool.h>
#include <stdint.h>

typedef struct Cell {
    bool state;
} Cell;

typedef struct Grid {
    Cell **cells;
    uint8_t rows;
    uint8_t cols;
} Grid;

void initCell(Cell *cell);

void initGrid(Grid *grid, uint8_t rows, uint8_t cols);
int gridDrawWidth(int scale, Grid grid);
int gridDrawHeight(int scale, Grid grid);
void drawGrid(int x, int y, int scale, int cellPadding, Grid grid);
void freeGrid(Grid *grid);

#endif // ptest_cell_h
