
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
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

QuadTree *quadrantToQuadtree(Quadrant quadrant, const QuadTree *quadtree) {
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

// -- QuadTree
void initQuadTree(QuadTree *quadtree, int depth) {
    quadtree->depth = depth;

    quadtree->NW = NULL;
    quadtree->NE = NULL;
    quadtree->SW = NULL;
    quadtree->SE = NULL;
}

QuadTree newQuadTree() {
    QuadTree quadtree;
    initQuadTree(&quadtree, 0);

    return quadtree;
}

void freeQuadrant(Quadrant quadrant, QuadTree *quadtree) {
    QuadTree *subtree = quadrantToQuadtree(quadrant, quadtree);
    if (subtree != NULL) {
        freeQuadTree(subtree);
        free(subtree);
    }
}

void freeQuadTree(QuadTree *quadtree) {
    freeQuadrant(NW, quadtree);
    freeQuadrant(NE, quadtree);
    freeQuadrant(SW, quadtree);
    freeQuadrant(SE, quadtree);
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
    initQuadTree(quadtree->NW, depth + 1);
    initQuadTree(quadtree->NE, depth + 1);
    initQuadTree(quadtree->SW, depth + 1);
    initQuadTree(quadtree->SE, depth + 1);

    return true;
}

Vector2 centerOfQuadrant(Quadrant quadrant, Vector2 center, float width) {
    switch (quadrant) {
    case NW:
        return (Vector2){center.x - width / 2.0f, center.y - width / 2.0f};
    case NE:
        return (Vector2){center.x + width / 2.0f, center.y - width / 2.0f};
    case SW:
        return (Vector2){center.x - width / 2.0f, center.y + width / 2.0f};
    case SE:
        return (Vector2){center.x + width / 2.0f, center.y + width / 2.0f};
    }
}

// TODO: REMOVE
bool inVisualQuadtree(Vector2 point, VisualQuadTree vquadtree) {
    float minx = vquadtree.center.x - vquadtree.width * 0.5f;
    float maxx = vquadtree.center.x + vquadtree.width * 0.5f;
    float miny = vquadtree.center.y - vquadtree.width * 0.5f;
    float maxy = vquadtree.center.y + vquadtree.width * 0.5f;

    return point.x >= minx && point.x <= maxx && point.y >= miny && point.y <= maxy;
}

QuadTree *quadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width) {
    if (!IN_SQUARE(point, center, width)) {
        return NULL;
    }

    QuadTree *subQuad = quadtree;
    while (isSubdivided(*subQuad)) {
        Quadrant quadrant = pointToQuadrant(point, center);
        subQuad = quadrantToQuadtree(quadrant, subQuad);
        center = centerOfQuadrant(quadrant, center, width/2.0f);
        width /= 2;
    }

    return subQuad;
}

void drawQuadTree(QuadTree quadtree, Vector2 center, float width, Camera2D camera) {
    if (isSubdivided(quadtree)) {
        drawQuadTree(*quadtree.NE, centerOfQuadrant(NE, center, width/2.0f), width/2.0f, camera);
        drawQuadTree(*quadtree.NW, centerOfQuadrant(NW, center, width/2.0f), width/2.0f, camera);
        drawQuadTree(*quadtree.SE, centerOfQuadrant(SE, center, width/2.0f), width/2.0f, camera);
        drawQuadTree(*quadtree.SW, centerOfQuadrant(SW, center, width/2.0f), width/2.0f, camera);
    }

    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
    drawCenteredSquare(center, 2, WHITE);
    drawCenteredSquareLines(center, width, WHITE);
}

void drawQuadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width) {
    if (!IN_SQUARE(point, center, width)) {
        return;
    }

    QuadTree *subQuad = quadtree;
    while (isSubdivided(*subQuad)) {
        Quadrant quadrant = pointToQuadrant(point, center);
        subQuad = quadrantToQuadtree(quadrant, subQuad);
        center = centerOfQuadrant(quadrant, center, width/2.0f);
        width /= 2;
    }

    drawCenteredSquareLines(center, width, BLUE);
    drawCenteredSquare(center, 2.0f, BLUE);
}

int maxQuads() { return pow(2, QUADTREE_MAX_DEPTH); }

float miniumumQuadSize(float width) { return width / (maxQuads()); }
