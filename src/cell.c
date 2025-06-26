#include <stdio.h>
#include <stdlib.h>

#include "cell.h"

typedef struct Neighbourhood {
    Cell *center;
    Cell *top;
    Cell *right;
    Cell *bottom;
    Cell *left;
} Neighbourhood;

void initCell(Cell *cell) { cell->state = false; }

void drawCell(int x, int y, int scale, int padding, Cell cell, Color color) {
    // TODO: Use cell state for color
    DrawRectangle(x + padding / 2, y + padding / 2, scale - padding, scale - padding, color);
}

void initGrid(Grid *grid, uint8_t rows, uint8_t cols, char *label) {
    grid->rows = rows;
    grid->cols = cols;

    grid->cells = malloc(rows * sizeof(Cell *));
    if (grid->cells == NULL) {
        // TODO: Memory limit reached error
        return;
    }

    for (int row = 0; row < rows; row++) {
        grid->cells[row] = malloc(cols * sizeof(Cell));
        if (grid->cells[row] == NULL) {
            // TODO: Memory limit reached error
            return;
        }
        for (int col = 0; col < cols; col++) {
            initCell(&grid->cells[row][col]);
        }
    }

    grid->label = label;
}

// Retruns the drawn width of the grid. Useful for centering a grid.
int gridDrawWidth(int scale, Grid grid) { return grid.cols * scale; }

// Retruns the drawn height of the grid. Useful for centering a grid.
int gridDrawHeight(int scale, Grid grid) { return grid.rows * scale; }

void drawGrid(int x, int y, int scale, int cellPadding, Grid grid) {
    // Background of grid
    // TODO: Add background color choice
    DrawRectangle(x, y, grid.cols * scale, grid.rows * scale, DARKGRAY);

    for (int row = 0; row < grid.rows; row++) {
        for (int col = 0; col < grid.cols; col++) {
            Cell cell = grid.cells[row][col];
            drawCell(x + col * scale, y + row * scale, scale, cellPadding, cell, cell.state ? BLUE : GRAY);
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

Neighbourhood getNeighbourhood(int row, int col, Grid *grid) {
    return (Neighbourhood){
        .center = &grid->cells[row][col],
        .top = row == 0 ? NULL : &grid->cells[row - 1][col],
        .right = col == grid->cols - 1 ? NULL : &grid->cells[row][col + 1],
        .bottom = row == grid->rows - 1 ? NULL : &grid->cells[row + 1][col],
        .left = col == 0 ? NULL : &grid->cells[row][col - 1],
    };
}

void updateNeighbourhoods(Neighbourhood neighbourhood1, Neighbourhood neighbourhood2) {
    neighbourhood2.center->state = neighbourhood1.center->state;

    if (neighbourhood1.top != NULL && neighbourhood1.top->state != false) {
        neighbourhood2.center->state = neighbourhood1.top->state;
        neighbourhood2.top->state = 0;
    }
}

void updateGrids(Grid *grid1, Grid *grid2) {
    if (grid1->rows != grid2->rows || grid1->cols != grid2->cols) {
        // TODO: Error
    }

    for (int i = 0; i < grid1->rows; i++) {
        for (int j = 0; j < grid2->rows; j++) {
            Neighbourhood neighbourhood1 = getNeighbourhood(i, j, grid1);
            Neighbourhood neighbourhood2 = getNeighbourhood(i, j, grid2);
            updateNeighbourhoods(neighbourhood1, neighbourhood2);
        }
    }
}

BoundingBox gridBoundingBox(int x, int y, int scale, Grid grid) {
    return (BoundingBox){
        .min = (Vector3){x, y, 0.0f},
        .max = (Vector3){grid.cols * scale + x, grid.rows * scale + y, 0.0f},
    };
}

Vector2 positionToGridLocation(float x, float y, int gridx, int gridy, int scale) {
    return (Vector2){
        (x - gridx) / scale,
        (y - gridy) / scale,
    };
}

Cell *positionToGridCell(int x, int y, int gridx, int gridy, int scale, const Grid *grid) {
    Vector2 location = positionToGridLocation(x, y, gridx, gridy, scale);
    int row = floorf(Clamp(location.y, 0, grid->rows));
    int col = floorf(Clamp(location.x, 0, grid->cols));

    return &grid->cells[row][col];
}
