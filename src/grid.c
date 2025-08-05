#include "grid.h"
#include "common.h"
#include "debug.h"
#include "memory.h"
#include "neighbourhood.h"

#include <math.h>

void initGrid(Grid *grid, uint16_t rows, uint16_t cols) {
    grid->rows = rows;
    grid->cols = cols;

    grid->cells = ALLOCATE(CellValue *, rows);

    for (int row = 0; row < rows; row++) {
        grid->cells[row] = ALLOCATE(CellValue, cols);
        for (int col = 0; col < cols; col++) {
            initCellValue(&grid->cells[row][col], VACUUM, NONE, 0);
        }
    }
}

Grid *newGrid(uint16_t rows, uint16_t cols) {
    Grid *grid;
    initGrid(grid, rows, cols);

    return grid;
}

void freeGrid(Grid *grid) {
    for (int row = 0; row < grid->rows; row++) {
        FREE(CellValue, grid->cells[row]);
    }

    FREE(CellValue *, grid->cells);
}

void drawGridPixels(const Grid *grid, int x, int y) {
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            DrawPixel(x + col, y + row, cellColor(grid->cells[row][col]));
        }
    }
}

void drawGrid(const Grid *grid, int x, int y, int cellWidth, int cellHeight, int spacing) {
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            drawCellValue(grid->cells[row][col], x + col * cellWidth, y + row * cellHeight, cellWidth, cellHeight);
        }
    }
}

static void copyGrid(const Grid *grid, Grid *result) {
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            copyCellValue(&grid->cells[row][col], &result->cells[row][col]);
        }
    }
}

// Returns the cell at position `row` and `col` in grid. Returns `_default` if the location is outside the grid.
static CellValue getCell(const Grid *grid, int row, int col, CellValue _default) {
    if ((0 <= row) && (row <= grid->rows - 1) && (0 <= col) && (col <= grid->cols - 1)) {
        return grid->cells[row][col];
    }
    return _default;
}

bool getCellAt(const Grid *grid, int grid_x, int grid_y, float x, float y, int cellWidth, int cellHeight,
               CellValue **result) {
    int row = floor((y - grid_y) / cellHeight);
    int col = floor((x - grid_x) / cellWidth);
    if (0 <= row && row < grid->rows && 0 <= col && col < grid->cols) {
        *result = &grid->cells[row][col];
        return true;
    }
    return false;
}

// Gets the cell neighbourhood of cell at `row` and `col`. `boundary` determines what objects outside the grid
// default to.
static CellNeighbourhood getCellNeighbourhood(const Grid *grid, int row, int col, CellValue boundary) {
    CellValue nw = getCell(grid, row - 1, col - 1, boundary);
    CellValue n = getCell(grid, row - 1, col, boundary);
    CellValue ne = getCell(grid, row - 1, col + 1, boundary);
    CellValue w = getCell(grid, row, col - 1, boundary);
    CellValue c = getCell(grid, row, col, boundary);
    CellValue e = getCell(grid, row, col + 1, boundary);
    CellValue sw = getCell(grid, row + 1, col - 1, boundary);
    CellValue s = getCell(grid, row + 1, col, boundary);
    CellValue se = getCell(grid, row + 1, col + 1, boundary);

    return newCellNeighbourhood(nw, n, ne, w, c, e, sw, s, se);
}

static void evolve(const Grid *grid, Grid *result) {
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            CellNeighbourhood n = getCellNeighbourhood(grid, row, col, newCellValue(SOLID, STONE, 1));
            grid->cells[row][col].occ = collide(n);
        }
    }

    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            CellNeighbourhood n = getCellNeighbourhood(grid, row, col, newCellValue(SOLID, STONE, 1));
            CellValue *cell = &result->cells[row][col];

            cell->state = surroundingSum(n);
            if (n.c.material != STONE) {
                if (cell->state == 0) {
                    cell->type = VACUUM;
                    cell->material = NONE;
                } else {
                    cell->type = FLUID;
                    cell->material = determineMaterial(n);
                }
            }

            n.c = *cell;
            *cell = react(n);
        }
    }
}

void evolveGrid(const Grid *grid, Grid *result) {
    if (grid->rows != result->rows || grid->cols != result->cols) {
        LogMessage(LOG_ERROR, "grid %p and result %p have misaligned columns or rows: grid (%d, %d), result (%d, %d)",
                   grid, result, grid->rows, grid->cols, result->rows, result->cols);
    }

    copyGrid(grid, result);
    evolve(grid, result);
}
