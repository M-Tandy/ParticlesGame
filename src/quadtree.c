
#include <stdio.h>
#include <stdlib.h>

#include "draw.h"
#include "quadtree.h"
#include "raymath.h"

// -- Point
typedef struct Point {
    int x;
    int y;
} Point;

Point newPoint(int x, int y) { return (Point){x, y}; }

// -- Node
typedef struct Node {
    Point pos;
    int data;
} Node;

typedef enum Quadrant {
    NW,
    NE,
    SW,
    SE,
} Quadrant;

Node newNode(Point pos, int data) { return (Node){pos, data}; }

// Quadrant
Quadrant pointToQuadrant(Vector2 point, Vector2 center) {
    int x = point.x - center.x;
    int y = point.y - center.y;

    if (x >= 0 && y >= 0) {
        return SE;
    } else if (x < 0 && y >= 0) {
        return SW;
    } else if (x < 0 && y < 0) {
        return NW;
    }

    return NE;
}

// -- QuadTree
void initQuadTree(QuadTree *quadtree, int depth, Vector2 center, float width) {
    quadtree->depth = depth;
    quadtree->center = center;
    quadtree->width = width;

    quadtree->NW = NULL;
    quadtree->NE = NULL;
    quadtree->SW = NULL;
    quadtree->SE = NULL;
}

QuadTree newQuadTree() {
    QuadTree quadtree;
    initQuadTree(&quadtree, 0, (Vector2){0.0f, 0.0f}, 5 * pow(2, QUADTREE_MAX_DEPTH));

    return quadtree;
}

void freeQuadTree(QuadTree *quadtree) {
    if (quadtree->NW != NULL) {
        freeQuadTree(quadtree->NW);
        free(quadtree->NW);
    }
    if (quadtree->NE != NULL) {
        freeQuadTree(quadtree->NE);
        free(quadtree->NE);
    }
    if (quadtree->SW != NULL) {
        freeQuadTree(quadtree->SW);
        free(quadtree->SW);
    }
    if (quadtree->SE != NULL) {
        freeQuadTree(quadtree->SE);
        free(quadtree->SE);
    }
}

QuadTree *quadtreeFromQuadrant(Quadrant quadrant, const QuadTree *quadtree) {
    switch (quadrant) {
    case NW:
        return quadtree->NW;
        break;
    case NE:
        return quadtree->NE;
        break;
    case SW:
        return quadtree->SW;
        break;
    case SE:
        return quadtree->SE;
        break;
    }
}

void drawQuadTree(QuadTree quadtree, Camera2D camera) {
    if (quadtree.NW != NULL) {
        drawQuadTree(*quadtree.NW, camera);
    }
    if (quadtree.NE != NULL) {
        drawQuadTree(*quadtree.NE, camera);
    }
    if (quadtree.SW != NULL) {
        drawQuadTree(*quadtree.SW, camera);
    }
    if (quadtree.SE != NULL) {
        drawQuadTree(*quadtree.SE, camera);
    }

    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
    drawCenteredSquare(quadtree.center, 2, WHITE);
    drawCenteredSquareLines(quadtree.center, quadtree.width, WHITE);
}

bool isSubdivided(QuadTree quadtree) { return quadtree.NE != NULL; }

// Subdivide the given quadtree, allocating memory and instantiating the 4 quadrants quad trees.
bool subdivideQuadTree(QuadTree *quadtree) {
    if (quadtree->depth == QUADTREE_MAX_DEPTH) {
        return false;
    }

    quadtree->NW = malloc(sizeof(QuadTree));
    quadtree->NE = malloc(sizeof(QuadTree));
    quadtree->SW = malloc(sizeof(QuadTree));
    quadtree->SE = malloc(sizeof(QuadTree));

    int depth = quadtree->depth;
    int width = quadtree->width / 2.0f;

    Vector2 upLeft = (Vector2){width / 2.0f, -width / 2.0f};
    Vector2 downRight = (Vector2){width / 2.0f, width / 2.0f};

    initQuadTree(quadtree->NW, depth + 1, Vector2Subtract(quadtree->center, downRight), width);
    initQuadTree(quadtree->NE, depth + 1, Vector2Add(quadtree->center, upLeft), width);
    initQuadTree(quadtree->SW, depth + 1, Vector2Subtract(quadtree->center, upLeft), width);
    initQuadTree(quadtree->SE, depth + 1, Vector2Add(quadtree->center, downRight), width);

    return true;
}

bool inQuad(Vector2 point, QuadTree quadtree) {
    float minx = quadtree.center.x - quadtree.width * 0.5f;
    float maxx = quadtree.center.x + quadtree.width * 0.5f;
    float miny = quadtree.center.y - quadtree.width * 0.5f;
    float maxy = quadtree.center.y + quadtree.width * 0.5f;

    return point.x >= minx && point.x <= maxx && point.y >= miny && point.y <= maxy;
}

QuadTree *quadFromPosition(Vector2 point, QuadTree *quadtree) {
    if (quadtree->NE != NULL) {
        Quadrant subquadrant = pointToQuadrant(point, quadtree->center);
        QuadTree *subtree = quadtreeFromQuadrant(subquadrant, quadtree);

        return quadFromPosition(point, subtree);
    }

    return quadtree;
}

int maxQuads(QuadTree quadtree) {
    return pow(2, QUADTREE_MAX_DEPTH);
}

float miniumumQuadSize(QuadTree quadtree) {
    return quadtree.width / (maxQuads(quadtree));
}

