
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "draw.h"
#include "hash.h"
#include "memory.h"
#include "quadtree.h"
#include "raymath.h"
#include "table.h"

typedef enum Quadrant {
    NW,
    NE,
    SW,
    SE,
} Quadrant;

Table quadtrees;

int hashQuad(QuadrantValue quad) {
    switch (quad.type) {

    case VAL_INT:
        return hash_6432shift(AS_INT(quad));
    case VAL_TREE:
        return hash_ptr(AS_QUADTREE(quad));
    case VAL_EMPTY:
        // Should never really be reached. We should not have an empty node in the tree
        return 0;
    }
}

int hashQuads(QuadrantValue nw, QuadrantValue ne, QuadrantValue sw, QuadrantValue se) {
    return hashQuad(nw) + 2 * hashQuad(ne) + 4 * hashQuad(sw) + 8 * hashQuad(se);
}

int hashQuadtree(const QuadTree *quadtree) { 
    return hashQuads(quadtree->NW, quadtree->NE, quadtree->SW, quadtree->SE); 
}

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

static QuadrantValue quadrantGet(Quadrant quadrant, const QuadTree *quadtree) {
    switch (quadrant) {
    case NW:
        return quadtree->NW;
    case NE:
        return quadtree->NE;
    case SW:
        return quadtree->SW;
    case SE:
        return quadtree->SE;
    }
}

void quadrantSet(Quadrant quadrant, QuadTree *quadtree, QuadrantValue value) {
    switch (quadrant) {
    case NW:
        quadtree->NW = value;
        break;
    case NE:
        quadtree->NE = value;
        break;
    case SW:
        quadtree->SW = value;
        break;
    case SE:
        quadtree->SE = value;
        break;
    }
}

void initQuadTable() { initTable(&quadtrees); }

static QuadTree *allocateQuadTree(QuadrantValue nw, QuadrantValue ne, QuadrantValue sw, QuadrantValue se, int depth,
                                  uint32_t hash) {
    QuadTree *quadtree = (QuadTree *)reallocate(NULL, 0, sizeof(QuadTree));
    quadtree->depth = depth;

    quadtree->NW = nw;
    quadtree->NE = ne;
    quadtree->SW = sw;
    quadtree->SE = se;

    quadtree->hash = hash;

    tableSet(&quadtrees, hash, quadtree);

    return quadtree;
}

// -- QuadTree
void initQuadTree(QuadTree *quadtree, int depth) {
    if (depth == QUADTREE_MAX_DEPTH) {
        quadtree->NW = INT_VALUE(0);
        quadtree->NE = INT_VALUE(0);
        quadtree->SW = INT_VALUE(0);
        quadtree->SE = INT_VALUE(0);
    } else {
        quadtree->NW = QUADTREE_VALUE(NULL);
        quadtree->NE = QUADTREE_VALUE(NULL);
        quadtree->SW = QUADTREE_VALUE(NULL);
        quadtree->SE = QUADTREE_VALUE(NULL);
    }
}

// TODO: Deprecate
QuadTree newQuadTree() {
    QuadTree quadtree;
    initQuadTree(&quadtree, 0);

    return quadtree;
}

// TODO: Deprecate
QuadTree newQuadTreeDepth(int depth) {
    QuadTree quadtree;
    initQuadTree(&quadtree, depth);

    return quadtree;
}

// TODO: Deprecate
void initEmptyQuad(QuadTree *quadtree, int depth) {
    quadtree->depth = depth;
    quadtree->NW = EMPTY_VALUE;
    quadtree->NE = EMPTY_VALUE;
    quadtree->SW = EMPTY_VALUE;
    quadtree->SE = EMPTY_VALUE;
}

static QuadTree leafNode(int nw, int ne, int sw, int se) {
    QuadTree quadtree =
        (QuadTree){.depth = 0, .NW = INT_VALUE(nw), .NE = INT_VALUE(ne), .SW = INT_VALUE(sw), .SE = INT_VALUE(se)};
    quadtree.hash = hashQuads(INT_VALUE(nw), INT_VALUE(ne), INT_VALUE(sw), INT_VALUE(se));
    return quadtree;
}

static bool isLeaf(QuadrantValue qvalue) { return IS_INT(qvalue); }

static QuadTree treeNode(int depth, const QuadTree *nw, const QuadTree *ne, const QuadTree *sw, const QuadTree *se) {
    QuadTree quadtree = (QuadTree){.depth = depth,
                                   .NW = QUADTREE_VALUE(nw),
                                   .NE = QUADTREE_VALUE(ne),
                                   .SW = QUADTREE_VALUE(sw),
                                   .SE = QUADTREE_VALUE(se)};
    quadtree.hash = hashQuads(QUADTREE_VALUE(nw), QUADTREE_VALUE(ne), QUADTREE_VALUE(sw), QUADTREE_VALUE(se));
    return quadtree;
}

static bool isNode(QuadrantValue qvalue) { return IS_QUADTREE(qvalue); }

// We will set 0 to be the lowest depth (leafs)
QuadTree *newEmptyQuadTree(int depth) {
    QuadTree emptyLeaf = leafNode(0, 0, 0, 0);

    // Check if the empty leaf is already interned
    QuadTree *quadtree = tableFindQuadTree(&quadtrees, &emptyLeaf, emptyLeaf.hash);
    if (quadtree == NULL) {
        quadtree = allocateQuadTree(INT_VALUE(0), INT_VALUE(0), INT_VALUE(0), INT_VALUE(0), 0, emptyLeaf.hash);
    }

    // Building the empty nodes from the leaf node upwards until the tree is full
    int i = 1;
    while (i < depth) {
        QuadTree newTree = treeNode(i, quadtree, quadtree, quadtree, quadtree);
        QuadTree *temp = tableFindQuadTree(&quadtrees, &newTree, newTree.hash); // BUG: Hash seems to be wrongly interned? Maybe?
        if (temp == NULL) {
            quadtree = allocateQuadTree(QUADTREE_VALUE(quadtree), QUADTREE_VALUE(quadtree), QUADTREE_VALUE(quadtree),
                                        QUADTREE_VALUE(quadtree), i, newTree.hash);
        } else {
            quadtree = temp;
        }
        i++;
    }

    return quadtree;
}

QuadTree copyQuadTree(const QuadTree *quadtree) {
    return (QuadTree){.depth = quadtree->depth,
                      .NW = quadtree->NW,
                      .NE = quadtree->NE,
                      .SW = quadtree->SW,
                      .SE = quadtree->SE,
                      .hash = quadtree->hash};
}

// TODO: Deprecate. Memory should now be managed separately.
void freeQuadrant(Quadrant quadrant, QuadTree *quadtree) {
    QuadrantValue qvalue = quadrantGet(quadrant, quadtree);
    if (!IS_QUADTREE(qvalue)) {
        return;
    }
    QuadTree *subtree = AS_QUADTREE(qvalue);
    if (subtree != NULL && isSubdivided(*subtree)) {
        freeQuadTree(subtree);
        free(subtree);
    }
}

// TODO: Deprecate. Memory should now be managed separately.
void freeQuadTree(QuadTree *quadtree) {
    freeQuadrant(NW, quadtree);
    freeQuadrant(NE, quadtree);
    freeQuadrant(SW, quadtree);
    freeQuadrant(SE, quadtree);
}

// Compare two `QuadrantValue`'s.
static bool compare(QuadrantValue left, QuadrantValue right) {
    if (left.type != right.type) {
        return false;
    }

    if (IS_QUADTREE(left)) {
        // If the quadtrees have the same pointer, they are equal. This should work due to interning
        return AS_QUADTREE(left) == AS_QUADTREE(right);
    } else {
        return AS_INT(left) == AS_INT(right);
    }
}

// Returns `true` if the quadtrees have the same values inside. Two `QuadTree` values are the same if they have the same
// pointer.
// TODO: Deprecate. Interning should make this simpler
bool quadtreesEqual(QuadTree *left, QuadTree *right) {
    if (left->depth != right->depth || !isSubdivided(*left) || !isSubdivided(*right)) {
        return false;
    }
    return compare(left->NW, right->NW) && compare(left->NE, right->NE) && compare(left->SW, right->SW) &&
           compare(left->SE, right->SE);
}

bool isSubdivided(QuadTree quadtree) { return !IS_QUADTREE(quadtree.NE) || AS_QUADTREE(quadtree.NE) != NULL; }

// Subdivide the given quadtree, allocating memory and instantiating the 4 quadrants quad trees.
bool subdivide(QuadTree *quadtree) {
    if (quadtree->depth == QUADTREE_MAX_DEPTH - 1) {
        quadtree->NW = INT_VALUE(0);
        quadtree->NE = INT_VALUE(0);
        quadtree->SW = INT_VALUE(0);
        quadtree->SE = INT_VALUE(0);

        return false;
    }

    quadtree->NW = QUADTREE_VALUE(malloc(sizeof(QuadTree)));
    quadtree->NE = QUADTREE_VALUE(malloc(sizeof(QuadTree)));
    quadtree->SW = QUADTREE_VALUE(malloc(sizeof(QuadTree)));
    quadtree->SE = QUADTREE_VALUE(malloc(sizeof(QuadTree)));

    int depth = quadtree->depth;
    initQuadTree(AS_QUADTREE(quadtree->NW), depth + 1);
    initQuadTree(AS_QUADTREE(quadtree->NE), depth + 1);
    initQuadTree(AS_QUADTREE(quadtree->SW), depth + 1);
    initQuadTree(AS_QUADTREE(quadtree->SE), depth + 1);

    return true;
}

void fullySubdivide(QuadTree *quadtree) {
    if (!isSubdivided(*quadtree)) {
        subdivide(quadtree);
    }

    QuadrantValue nw = quadtree->NW;
    QuadrantValue ne = quadtree->NE;
    QuadrantValue sw = quadtree->SW;
    QuadrantValue se = quadtree->SE;

    if IS_QUADTREE (nw)
        fullySubdivide(AS_QUADTREE(nw));
    if IS_QUADTREE (ne)
        fullySubdivide(AS_QUADTREE(ne));
    if IS_QUADTREE (sw)
        fullySubdivide(AS_QUADTREE(sw));
    if IS_QUADTREE (se)
        fullySubdivide(AS_QUADTREE(se));
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

QuadrantValue flip(QuadrantValue value) {
    return INT_VALUE((AS_INT(value) + 1) % 2);
}

// Set's leaf value at the given point in space to the given value, and returns the pointer with that value.
// This doesn't edit the quadtree, just returns the quadtree with that pointer as value
QuadTree *setPointInQuadTree(Vector2 point, Vector2 center, float width, const QuadTree *quadtree,
                             QuadrantValue value) {
    if (!IN_SQUARE(point, center, width)) {
        // point outside of quadtree so do nothing
        return NULL;
    }

    // Find the quadrant the point is located in
    Quadrant quadrant = pointToQuadrant(point, center);
    QuadrantValue qvalue = quadrantGet(quadrant, quadtree);

    QuadTree copy;
    if (isLeaf(qvalue)) {
        // Base Case - This quadtree is a leaf node, and qvalue is an integer
        copy = copyQuadTree(quadtree);
        QuadrantValue newValue = AS_INT(value) == -1 ? flip(quadrantGet(quadrant, &copy)) : value;
        quadrantSet(quadrant, &copy, newValue); // TODO: We use -1 to represent a flip. Integer overflow will case a problem?
    } else {
        // Recursively go down until at a leaf
        center = centerOfQuadrant(quadrant, center, width / 2.0f);
        width /= 2;

        QuadTree *subTree = setPointInQuadTree(point, center, width, AS_QUADTREE(qvalue), value);
        copy = copyQuadTree(quadtree);
        quadrantSet(quadrant, &copy, QUADTREE_VALUE(subTree));
    }

    copy.hash = hashQuadtree(&copy);

    // Check if the tree is interned
    QuadTree *interned = tableFindQuadTree(&quadtrees, &copy, copy.hash);
    if (interned == NULL) {
        interned = allocateQuadTree(copy.NW, copy.NE, copy.SW, copy.SE, copy.depth, copy.hash);
    }

    return interned;
}

// Assumes that the quadtree has been subdivided at least once. Returns the QuadrantValue located at `point`
// assuming the quadtree is draw at `center` with size `width`
QuadrantValue *quadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width) {
    if (!IN_SQUARE(point, center, width)) {
        return NULL;
    }

    if (!isSubdivided(*quadtree)) {
        return &QUADTREE_VALUE(quadtree);
    }

    QuadTree *subQuad = quadtree;
    QuadrantValue value;
    while (isSubdivided(*subQuad)) {
        Quadrant quadrant = pointToQuadrant(point, center);
        value = quadrantGet(quadrant, subQuad);
        if (IS_INT(value)) {
            return &value;
        }
        subQuad = AS_QUADTREE(value);
        center = centerOfQuadrant(quadrant, center, width / 2.0f);
        width /= 2;
    }

    return &value;
}

void drawQuadTreeOld(QuadTree quadtree, Vector2 center, float width, Camera2D camera) {

#define DRAW_QUAD(tree, quad)                                                                                          \
    (drawQuadTree(*AS_QUADTREE(tree.quad), centerOfQuadrant(quad, center, width / 2.0f), width / 2.0f, camera))
#define DRAW_INT(tree, quad)                                                                                           \
    (drawCenteredSquare(centerOfQuadrant(quad, center, width / 2.0f), width / 2.0f,                                    \
                        AS_INT(tree.quad) == 0 ? BLACK : WHITE))

    if (isSubdivided(quadtree)) {
        if (IS_QUADTREE(quadtree.NE)) {
            DRAW_QUAD(quadtree, NW);
            DRAW_QUAD(quadtree, NE);
            DRAW_QUAD(quadtree, SW);
            DRAW_QUAD(quadtree, SE);

#ifdef DEBUG_QUADINFO
            Vector2 textPos = centerOfQuadrant(NW, center, width / 2.0f);
            DrawText(TextFormat("%p", AS_QUADTREE(quadtree.NW)), textPos.x, textPos.y, 16, WHITE);
            textPos = centerOfQuadrant(NE, center, width / 2.0f);
            DrawText(TextFormat("%p", AS_QUADTREE(quadtree.NE)), textPos.x, textPos.y, 16, WHITE);
            textPos = centerOfQuadrant(SW, center, width / 2.0f);
            DrawText(TextFormat("%p", AS_QUADTREE(quadtree.SW)), textPos.x, textPos.y, 16, WHITE);
            textPos = centerOfQuadrant(SE, center, width / 2.0f);
            DrawText(TextFormat("%p", AS_QUADTREE(quadtree.SE)), textPos.x, textPos.y, 16, WHITE);
#endif
        } else {
            DRAW_INT(quadtree, NW);
            DRAW_INT(quadtree, NE);
            DRAW_INT(quadtree, SW);
            DRAW_INT(quadtree, SE);
        }
    }

#ifdef DEBUG_DRAWQUADS
    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
    drawCenteredSquare(center, 2, GREEN);
    drawCenteredSquareLines(center, width, GREEN);

#endif // DEBUG_DRAWQUADS

#undef DRAW_QUAD
#undef DRAW_INT
}

void drawQuadTree(QuadTree quadtree, Vector2 center, float width, Camera2D camera) {

#define DRAW_QUAD(tree, quad)                                                                                          \
    (drawQuadTree(*AS_QUADTREE(tree.quad), centerOfQuadrant(quad, center, width / 2.0f), width / 2.0f, camera))
#define DRAW_INT(tree, quad)                                                                                           \
    (drawCenteredSquare(centerOfQuadrant(quad, center, width / 2.0f), 0.9f * width / 2.0f,                             \
                        AS_INT(tree.quad) == 0 ? GRAY : BLUE))

    if (isNode(quadtree.NW)) {
        DRAW_QUAD(quadtree, NW);
        DRAW_QUAD(quadtree, NE);
        DRAW_QUAD(quadtree, SW);
        DRAW_QUAD(quadtree, SE);

#ifdef DEBUG_QUADINFO
        Vector2 textPos = centerOfQuadrant(NW, center, width / 2.0f);
        DrawText(TextFormat("%p \n %lu", AS_QUADTREE(quadtree.NW), AS_QUADTREE(quadtree.NW)->hash), textPos.x, textPos.y, 12, WHITE);
        textPos = centerOfQuadrant(NE, center, width / 2.0f);
        DrawText(TextFormat("%p \n %lu", AS_QUADTREE(quadtree.NE), AS_QUADTREE(quadtree.NE)->hash), textPos.x, textPos.y, 12, WHITE);
        textPos = centerOfQuadrant(SW, center, width / 2.0f);
        DrawText(TextFormat("%p \n %lu", AS_QUADTREE(quadtree.SW), AS_QUADTREE(quadtree.SW)->hash), textPos.x, textPos.y, 12, WHITE);
        textPos = centerOfQuadrant(SE, center, width / 2.0f);
        DrawText(TextFormat("%p \n %lu", AS_QUADTREE(quadtree.SE), AS_QUADTREE(quadtree.SE)->hash), textPos.x, textPos.y, 12, WHITE);
#endif
    } else if (isLeaf(quadtree.NW)) {
        DRAW_INT(quadtree, NW);
        DRAW_INT(quadtree, NE);
        DRAW_INT(quadtree, SW);
        DRAW_INT(quadtree, SE);
    }

#ifdef DEBUG_DRAW_QUADS
    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
    drawCenteredSquare(center, 2, GREEN);
    drawCenteredSquareLines(center, width, GREEN);

#endif // DEBUG_DRAWQUADS

#undef DRAW_QUAD
#undef DRAW_INT
}

void drawQuadFromPosition(Vector2 point, QuadTree *quadtree, Vector2 center, float width) {
    if (!IN_SQUARE(point, center, width)) {
        return;
    }

    QuadTree *subQuad = quadtree;
    while (isSubdivided(*subQuad)) {
        Quadrant quadrant = pointToQuadrant(point, center);
        if (!IS_QUADTREE(quadrantGet(quadrant, subQuad))) {
            break;
        }
        subQuad = AS_QUADTREE(quadrantGet(quadrant, subQuad));
        center = centerOfQuadrant(quadrant, center, width / 2.0f);
        width /= 2;
    }

    drawCenteredSquareLines(center, width, BLUE);
    drawCenteredSquare(center, 2.0f, BLUE);
}

int maxQuads() { return pow(2, QUADTREE_MAX_DEPTH + 1); }

float miniumumQuadSize(float width) { return width / (maxQuads()); }

typedef struct CellNeighbourhood {
    int nw;
    int n;
    int ne;
    int w;
    int c;
    int e;
    int sw;
    int s;
    int se;
} CellNeighbourhood;

CellNeighbourhood fromQuadrantValues(QuadrantValue nw, QuadrantValue n, QuadrantValue ne, QuadrantValue w,
                                     QuadrantValue c, QuadrantValue e, QuadrantValue sw, QuadrantValue s,
                                     QuadrantValue se) {
    return (CellNeighbourhood){AS_INT(nw), AS_INT(n),  AS_INT(ne), AS_INT(w), AS_INT(c),
                               AS_INT(e),  AS_INT(sw), AS_INT(s),  AS_INT(se)};
}

int surroundingSum(CellNeighbourhood n) { return n.nw + n.n + n.ne + n.w + n.e + n.sw + n.s + n.se; }

// Game of Life
int gameOfLife(CellNeighbourhood n) {
    int count = surroundingSum(n);
    if (n.c == 0) {
        // Dead cell
        return count == 3;
    }
    // Live cell
    return count == 2 || count == 3;
}

void evolveBaseCase(QuadTree *quadtree, QuadTree *result) {
    QuadTree *nw = AS_QUADTREE(quadtree->NW);
    QuadTree *ne = AS_QUADTREE(quadtree->NE);
    QuadTree *sw = AS_QUADTREE(quadtree->SW);
    QuadTree *se = AS_QUADTREE(quadtree->SE);

    QuadTree *r_nw = AS_QUADTREE(result->NW);
    QuadTree *r_ne = AS_QUADTREE(result->NE);
    QuadTree *r_sw = AS_QUADTREE(result->SW);
    QuadTree *r_se = AS_QUADTREE(result->SE);

    CellNeighbourhood n;
    if (nw != NULL) {
        n = fromQuadrantValues(nw->NW, nw->NE, ne->NW, nw->SW, nw->SE, ne->SW, sw->NW, sw->NE, se->NW);
        AS_INT(r_nw->SE) = gameOfLife(n);
    }

    if (ne != NULL) {
        n = fromQuadrantValues(nw->NE, ne->NW, ne->NE, nw->SE, ne->SW, ne->SE, sw->NE, se->NW, se->NE);
        AS_INT(r_ne->SW) = gameOfLife(n);
    }

    if (sw != NULL) {
        n = fromQuadrantValues(nw->SW, nw->SE, ne->SW, sw->NW, sw->NE, se->NW, sw->SW, sw->SE, se->SW);
        AS_INT(r_sw->NE) = gameOfLife(n);
    }

    if (se != NULL) {
        n = fromQuadrantValues(nw->SE, ne->SW, ne->SE, sw->NE, se->NW, se->NE, sw->SE, se->SW, se->SE);
        AS_INT(r_se->NW) = gameOfLife(n);
    }
}

QuadTree LRDummyQuadTree(QuadTree *left, QuadTree *right) {
    QuadTree dummy = newQuadTreeDepth(left->depth);
    dummy.NW = left->NE;
    dummy.SW = left->SE;
    dummy.NE = right->NW;
    dummy.SE = right->SW;

    return dummy;
}

QuadTree UDDummyQuadTree(QuadTree *up, QuadTree *down) {
    QuadTree dummy = newQuadTreeDepth(up->depth);
    dummy.NW = up->SW;
    dummy.NE = up->SE;
    dummy.SW = down->NW;
    dummy.SE = down->NE;

    return dummy;
}

QuadTree CenterDummyQuadTree(QuadTree *quadtree) {
    QuadTree dummy = newQuadTreeDepth(quadtree->depth + 1);
    dummy.NW = AS_QUADTREE(quadtree->NW)->SE;
    dummy.NE = AS_QUADTREE(quadtree->NE)->SW;
    dummy.SW = AS_QUADTREE(quadtree->SW)->NE;
    dummy.SE = AS_QUADTREE(quadtree->SE)->NW;

    return dummy;
}

void evolve(QuadTree *quadtree, QuadTree *result) {
    if (quadtree->depth == QUADTREE_MAX_DEPTH - 1) {
        evolveBaseCase(quadtree, result);
    } else {
        QuadTree *nw = AS_QUADTREE(quadtree->NW);
        QuadTree *ne = AS_QUADTREE(quadtree->NE);
        QuadTree *sw = AS_QUADTREE(quadtree->SW);
        QuadTree *se = AS_QUADTREE(quadtree->SE);

        QuadTree *nw_result = AS_QUADTREE(result->NW);
        QuadTree *ne_result = AS_QUADTREE(result->NE);
        QuadTree *sw_result = AS_QUADTREE(result->SW);
        QuadTree *se_result = AS_QUADTREE(result->SE);

        // Quadrant centers
        evolve(nw, nw_result);
        evolve(ne, ne_result);
        evolve(sw, sw_result);
        evolve(se, se_result);

        QuadTree dummyNode;
        QuadTree resultNode;

        // North dummy
        dummyNode = LRDummyQuadTree(nw, ne);
        resultNode = LRDummyQuadTree(nw_result, ne_result);
        evolve(&dummyNode, &resultNode);

        // South dummy
        dummyNode = LRDummyQuadTree(sw, se);
        resultNode = LRDummyQuadTree(sw_result, se_result);
        evolve(&dummyNode, &resultNode);

        // West dummy
        dummyNode = UDDummyQuadTree(nw, sw);
        resultNode = UDDummyQuadTree(nw_result, sw_result);
        evolve(&dummyNode, &resultNode);

        // East dummy
        dummyNode = UDDummyQuadTree(ne, se);
        resultNode = UDDummyQuadTree(ne_result, se_result);
        evolve(&dummyNode, &resultNode);

        // Center dummy
        dummyNode = CenterDummyQuadTree(quadtree);
        resultNode = CenterDummyQuadTree(result);
        evolve(&dummyNode, &resultNode);
    }
}

void evolveQuadtree(QuadTree *quadtree, QuadTree *result) {
    QuadTree dummy = newQuadTreeDepth(-1);
    QuadTree resultDummy = newQuadTreeDepth(-1);

    fullySubdivide(&dummy);
    fullySubdivide(&resultDummy);

    AS_QUADTREE(dummy.NW)->SE = quadrantGet(NW, quadtree);
    AS_QUADTREE(dummy.NE)->SW = quadrantGet(NE, quadtree);
    AS_QUADTREE(dummy.SW)->NE = quadrantGet(SW, quadtree);
    AS_QUADTREE(dummy.SE)->NW = quadrantGet(SE, quadtree);

    AS_QUADTREE(resultDummy.NW)->SE = quadrantGet(NW, result);
    AS_QUADTREE(resultDummy.NE)->SW = quadrantGet(NE, result);
    AS_QUADTREE(resultDummy.SW)->NE = quadrantGet(SW, result);
    AS_QUADTREE(resultDummy.SE)->NW = quadrantGet(SE, result);

    evolve(&dummy, &resultDummy);

    // TODO: Memory leek here. Need to free quadrants in dummy and result dummy that are not the center!
}
