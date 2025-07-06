#ifndef ptest_quadtree_h
#define ptest_quadtree_h

#include "raylib.h"

#define QUADTREE_MAX_DEPTH 6

typedef struct QuadTree QuadTree;

typedef enum {
    VAL_INT,
    VAL_TREE,
    VAL_EMPTY,
} ValueType;

typedef struct QuadrantValue {
    ValueType type;
    union {
        int integer;
        QuadTree *quadtree;
    } as;
} QuadrantValue;

#define IS_INT(qvalue)      (((qvalue).type) == VAL_INT)
#define IS_QUADTREE(qvalue) (((qvalue).type) == VAL_TREE)
#define IS_EMPTY(qvalue)    (((qvalue).type) == VAL_EMPTY)

#define AS_INT(qvalue)      ((qvalue).as.integer)
#define AS_QUADTREE(qvalue) ((qvalue).as.quadtree)

#define INT_VALUE(value)         ((QuadrantValue){VAL_INT, { .integer = value }})
#define QUADTREE_VALUE(qtree)    ((QuadrantValue){VAL_TREE, { .quadtree = (QuadTree *)qtree }})
#define EMPTY_VALUE              ((QuadrantValue){VAL_TREE, { .quadtree = (QuadTree *)NULL }})

typedef struct QuadTree {
    int depth;

    struct QuadrantValue NW;
    struct QuadrantValue NE;
    struct QuadrantValue SW;
    struct QuadrantValue SE;
} QuadTree;

#define GET_QUADRANT(quadtree, value) ((quadtree).value)

void initQuadTree(QuadTree *quadtree, int depth);
QuadTree newQuadTree();
void freeQuadTree(QuadTree *quadtree);
bool isSubdivided(QuadTree quadtree);
bool subdivide(QuadTree *quadtree);
void fullySubdivide(QuadTree *quadtree);
QuadrantValue *quadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width);
void drawQuadTree(QuadTree quadtree, Vector2 center, float width, Camera2D camera);
void drawQuadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width);
int maxQuads();
float miniumumQuadSize(float width);

void evolve(QuadTree *quadtree, QuadTree *result);
void evolveQuadtree(QuadTree *quadtree, QuadTree *result);
#endif // ptest_quadtree_h
