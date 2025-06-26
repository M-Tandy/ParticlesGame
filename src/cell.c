#include <stdio.h>
#include <stdlib.h>

#include "cell.h"
#include "raylib.h"

void initCell(Cell *cell) { cell->state = false; }

void drawCell(int x, int y, int scale, int padding, Cell cell, Color color) {
    // TODO: Use cell state for color
    DrawRectangle(x + padding / 2, y + padding / 2, scale - padding , scale - padding , color);
}

void initGrid(Grid *grid, uint8_t rows, uint8_t cols) {
    grid->rows = rows;
    grid->cols = cols;

    grid->cells = malloc(rows * sizeof(Cell *));
    if (grid->cells == NULL) {
        // TODO: Memory limit reached error
        return;
    }

    for (int i = 0; i < rows; i++) {
        grid->cells[i] = malloc(cols * sizeof(Cell));
        if (grid->cells[i] == NULL) {
            // TODO: Memory limit reached error
            return;
        }
        for (int j = 0; j < cols; j++) {
            initCell(&grid->cells[i][j]);
        }
    }
}

// Retruns the drawn width of the grid. Useful for centering a grid.
int gridDrawWidth(int scale, Grid grid) { return grid.cols * scale; }

// Retruns the drawn height of the grid. Useful for centering a grid.
int gridDrawHeight(int scale, Grid grid) { return grid.rows * scale; }

void drawGrid(int x, int y, int scale, int cellPadding, Grid grid) {
    // Background of grid
    // TODO: Add background color choice
    DrawRectangle(x, y, grid.cols * scale, grid.rows * scale, DARKGRAY);

    for (int i = 0; i < grid.rows; i++) {
        for (int j = 0; j < grid.cols; j++) {
            Cell cell = grid.cells[i][j];
            drawCell(x + i * scale, y + j * scale, scale, cellPadding, cell, GRAY);
        }
    }
}

void freeGrid(Grid *grid) {
    for (int i = 0; i < grid->rows; i++) {
        free(grid->cells[i]);
        grid->cells[i] = NULL;
    }
    free(grid->cells);
    grid->cells = NULL;

    grid->rows = 0;
    grid->cols = 0;
}
