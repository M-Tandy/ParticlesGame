#ifndef ptest_quadtree_h
#define ptest_quadtree_h

#include "raylib.h"

#define QUADTREE_MAX_DEPTH 7

typedef struct QuadTree {
    int depth;

    struct QuadTree *NW;
    struct QuadTree *NE;
    struct QuadTree *SW;
    struct QuadTree *SE;
} QuadTree;

typedef struct VisualQuadTree {
    QuadTree quadtree;
    Vector2 center;
    float width;
} VisualQuadTree;

QuadTree newQuadTree();
bool isSubdivided(QuadTree quadtree);
bool subdivideQuadTree(QuadTree *quadtree);
void freeQuadTree(QuadTree *quadtree);
QuadTree *quadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width);
void drawQuadTree(QuadTree quadtree, Vector2 center, float width, Camera2D camera);
void drawQuadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width);
int maxQuads();
float miniumumQuadSize(float width);

#endif // ptest_quadtree_h
