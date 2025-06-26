#include <stdio.h>

#include "cell.h"

int mainOld() {
    Grid grid;
    initGrid(&grid, 5, 5);

    drawGrid(&grid);

    freeGrid(&grid);

    return 0;
}
