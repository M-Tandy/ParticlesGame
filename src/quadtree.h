#ifndef ptest_quadtree_h
#define ptest_quadtree_h

#include "raylib.h"

#define QUADTREE_MAX_DEPTH 7

typedef struct QuadTree {
    int depth;
    Vector2 center;
    float width;

    struct QuadTree *NW;
    struct QuadTree *NE;
    struct QuadTree *SW;
    struct QuadTree *SE;
} QuadTree;

QuadTree newQuadTree();
bool isSubdivided(QuadTree quadtree);
bool subdivideQuadTree(QuadTree *quadtree);
void freeQuadTree(QuadTree *quadtree);
void drawQuadTree(QuadTree quadtree, Camera2D camera);
QuadTree *quadFromPosition(Vector2 point, QuadTree *quadtree);
int maxQuads(QuadTree quadtree);
float miniumumQuadSize(QuadTree quadtree);

#endif // ptest_quadtree_h
