#ifndef ptest_quadtree_h
#define ptest_quadtree_h

#include <stdint.h>

#include "raylib.h"

#define QUADTREE_MAX_DEPTH 6

typedef struct QuadTree QuadTree;

typedef struct QOccupationNumber {
    int nw;
    int n;
    int ne;
    int w;
    int c;
    int e;
    int sw;
    int s;
    int se;
} QOccupationNumber;

typedef enum {
    FLUID_WATER,
} FluidType;

typedef struct FluidValue {
    FluidType type;
    int state; // value from 0 to 10
} FluidValue;

typedef enum {
    VAL_INT,
    VAL_FLUID,
    VAL_TREE,
    VAL_OCCUPATION,
    VAL_EMPTY,
} ValueType;

typedef struct QuadrantValue {
    ValueType type;
    union {
        int integer;
        FluidValue fluid;
        QuadTree *quadtree;
        QOccupationNumber occupationNumber;
    } as;
} QuadrantValue;

#define IS_INT(qvalue) (((qvalue).type) == VAL_INT)
#define IS_FLUID(qvalue) (((qvalue).type) == VAL_FLUID)
#define IS_QUADTREE(qvalue) (((qvalue).type) == VAL_TREE)
#define IS_OCCUPATION_NUMBER(qvalue) (((qvalue).type) == VAL_OCCUPATION)
#define IS_EMPTY(qvalue) (((qvalue).type) == VAL_EMPTY)

#define AS_INT(qvalue) ((qvalue).as.integer)
#define AS_FLUID(qvalue) ((qvalue).as.fluid)
#define AS_OCCUPATION_NUMBER(qvalue) ((qvalue).as.occupationNumber)
#define AS_QUADTREE(qvalue) ((qvalue).as.quadtree)

#define INT_VALUE(value) ((QuadrantValue){VAL_INT, {.integer = value}})
#define FLUID_VALUE(fvalue) ((QuadrantValue){VAL_FLUID, {.fluid = (FluidValue)fvalue}})
#define QUADTREE_VALUE(qtree) ((QuadrantValue){VAL_TREE, {.quadtree = (QuadTree *)qtree}})
#define OCCUPATION_NUMBER_VALUE(occ) ((QuadrantValue){VAL_OCCUPATION, {.occupationNumber = (QOccupationNumber)occ}})
#define EMPTY_VALUE ((QuadrantValue){VAL_TREE, {.quadtree = (QuadTree *)NULL}})

typedef struct CellNeighbourhood {
    QuadrantValue nw;
    QuadrantValue n;
    QuadrantValue ne;
    QuadrantValue w;
    QuadrantValue c;
    QuadrantValue e;
    QuadrantValue sw;
    QuadrantValue s;
    QuadrantValue se;
} CellNeighbourhood;

typedef struct QuadTree {
    int depth;

    struct QuadrantValue NW;
    struct QuadrantValue NE;
    struct QuadrantValue SW;
    struct QuadrantValue SE;

    uint32_t hash;

    QuadTree *result;
} QuadTree;

#define GET_QUADRANT(quadtree, value) ((quadtree).value)

void printTreeTable();
void initQuadTable();

bool quadtreesEqual(const QuadTree *left, const QuadTree *right);
bool isSubdivided(QuadTree quadtree);

QuadTree *newEmptyQuadTree(int depth);
QuadTree *setPointInQuadTree(Vector2 point, Vector2 center, float width, const QuadTree *quadtree, QuadrantValue value);

void drawQuadTree(QuadTree quadtree, Vector2 center, float width, Camera2D camera);
void drawQuadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width);

int maxQuads(const QuadTree *quadtree);
float miniumumQuadSize(float width, const QuadTree *quadtree);

QuadTree *evolveQuadtree(const QuadTree *quadtree);
#endif // ptest_quadtree_h
