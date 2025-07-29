#ifndef ptest_fluid_h
#define ptest_fluid_h

#include "quadtree.h"

typedef enum {
    WATER,
} Type;

typedef struct Fluid {
    Type type;
    int state; // value from 0 to 10
} Fluid;

QuadrantValue fluidNeighbourhood(CellNeighbourhood n);
#endif // ptest_fluid_h
