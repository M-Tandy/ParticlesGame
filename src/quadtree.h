#ifndef ptest_quadtree_h
#define ptest_quadtree_h

#include <stdint.h>

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

#define IS_INT(qvalue) (((qvalue).type) == VAL_INT)
#define IS_QUADTREE(qvalue) (((qvalue).type) == VAL_TREE)
#define IS_EMPTY(qvalue) (((qvalue).type) == VAL_EMPTY)

#define AS_INT(qvalue) ((qvalue).as.integer)
#define AS_QUADTREE(qvalue) ((qvalue).as.quadtree)

#define INT_VALUE(value) ((QuadrantValue){VAL_INT, {.integer = value}})
#define QUADTREE_VALUE(qtree) ((QuadrantValue){VAL_TREE, {.quadtree = (QuadTree *)qtree}})
#define EMPTY_VALUE ((QuadrantValue){VAL_TREE, {.quadtree = (QuadTree *)NULL}})

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

QuadTree *newEmptyQuadTree(int depth);
void initQuadTable();
QuadTree *setPointInQuadTree(Vector2 point, Vector2 center, float width, const QuadTree *quadtree, QuadrantValue value);
QuadTree *evolveQuadtree(const QuadTree *quadtree);

QuadTree newQuadTree();
void freeQuadTree(QuadTree *quadtree);
bool quadtreesEqual(const QuadTree *left, const QuadTree *right);
bool isSubdivided(QuadTree quadtree);
bool subdivide(QuadTree *quadtree);
void fullySubdivide(QuadTree *quadtree);
QuadrantValue *quadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width);
void drawQuadTree(QuadTree quadtree, Vector2 center, float width, Camera2D camera);
void drawQuadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width);
int maxQuads(const QuadTree *quadtree);
float miniumumQuadSize(float width, const QuadTree *quadtree);

QuadTree *evolveQuadtree(const QuadTree *quadtree);
#endif // ptest_quadtree_h
