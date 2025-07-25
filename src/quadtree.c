#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "debug.h"
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

// QuadTree table
void printTreeTable() { tablePrint(&quadtrees); }
void initQuadTable() { initTable(&quadtrees); }

// Quadrant
static Quadrant pointToQuadrant(Vector2 point, Vector2 center) {
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

static void quadrantSet(Quadrant quadrant, QuadTree *quadtree, QuadrantValue value) {
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

// Hashing
static int hashQvalue(QuadrantValue qvalue) {
    switch (qvalue.type) {

    case VAL_INT:
        return hash_6432shift(AS_INT(qvalue));
    case VAL_FLUID: {
        FluidValue fluid = AS_FLUID(qvalue);
        return hash_6432shift(fluid.type + fluid.state);
    }
    case VAL_TREE:
        return hash_ptr(AS_QUADTREE(qvalue));
    case VAL_EMPTY:
        // Should never really be reached. We should not have an empty node in the tree
        return 0;
    }
}

static int hashQuadrants(QuadrantValue nw, QuadrantValue ne, QuadrantValue sw, QuadrantValue se) {
    return hashQvalue(nw) + 2 * hashQvalue(ne) + 4 * hashQvalue(sw) + 8 * hashQvalue(se);
}

static int hashQuadTree(const QuadTree *quadtree) {
    return hashQuadrants(quadtree->NW, quadtree->NE, quadtree->SW, quadtree->SE);
}

// QuadrantValues's

// Compare two `QuadrantValue`'s.
static bool compare(QuadrantValue left, QuadrantValue right) {
    if (left.type != right.type) {
        return false;
    }

    switch (left.type) {
    case VAL_INT:
        return AS_INT(left) == AS_INT(right);
    case VAL_FLUID: {
        FluidValue leftFluid = AS_FLUID(left);
        FluidValue rightFluid = AS_FLUID(right);
        return leftFluid.type == rightFluid.type && leftFluid.state == rightFluid.state;
    }
    case VAL_TREE:
        // If the quadtrees have the same pointer, they are equal. This should work due to interning
        return AS_QUADTREE(left) == AS_QUADTREE(right);
    case VAL_EMPTY:
        return true;
    }
}

// Flips an integer quadrant value between 0 and 1
static QuadrantValue flip(QuadrantValue value) { return INT_VALUE((AS_INT(value) + 1) % 2); }

static bool isEmpty(QuadrantValue qvalue) { return IS_INT(qvalue) && AS_INT(qvalue) == 0; }

// QuadTrees

// Allocate a quadtree on the heap with the given quadrant values
static QuadTree *allocateQuadTree(QuadrantValue nw, QuadrantValue ne, QuadrantValue sw, QuadrantValue se, int depth,
                                  uint32_t hash) {
    QuadTree *quadtree = (QuadTree *)reallocate(NULL, 0, sizeof(QuadTree));
    quadtree->depth = depth;

    quadtree->NW = nw;
    quadtree->NE = ne;
    quadtree->SW = sw;
    quadtree->SE = se;

    quadtree->hash = hash;

    quadtree->result = NULL;

    tableSet(&quadtrees, hash, quadtree);

    return quadtree;
}

// Attempts to copy the tree the the heap. Returns the interned tree if it already exists.
static QuadTree *copyQuadTree(QuadTree *quadtree) {
    QuadTree *interned = tableFindQuadTree(&quadtrees, quadtree, quadtree->hash);
    if (interned == NULL) {
        interned =
            allocateQuadTree(quadtree->NW, quadtree->NE, quadtree->SW, quadtree->SE, quadtree->depth, quadtree->hash);
    }

    return interned;
}

// Returns `true` if the quadtrees have the same values inside. Two `QuadTree` values are the same if they have the same
// pointer.
bool quadtreesEqual(const QuadTree *left, const QuadTree *right) {
    if (left->depth != right->depth || !isSubdivided(*left) || !isSubdivided(*right)) {
        return false;
    }
    return compare(left->NW, right->NW) && compare(left->NE, right->NE) && compare(left->SW, right->SW) &&
           compare(left->SE, right->SE);
}

bool isSubdivided(QuadTree quadtree) { return !IS_QUADTREE(quadtree.NE) || AS_QUADTREE(quadtree.NE) != NULL; }

static QuadTree leafNode(int nw, int ne, int sw, int se) {
    QuadTree quadtree =
        (QuadTree){.depth = 1, .NW = INT_VALUE(nw), .NE = INT_VALUE(ne), .SW = INT_VALUE(sw), .SE = INT_VALUE(se)};
    quadtree.hash = hashQuadrants(INT_VALUE(nw), INT_VALUE(ne), INT_VALUE(sw), INT_VALUE(se));
    quadtree.result = NULL;
    return quadtree;
}

static bool isLeaf(QuadrantValue qvalue) { return IS_INT(qvalue) || IS_FLUID(qvalue); }

static QuadTree treeNode(int depth, const QuadTree *nw, const QuadTree *ne, const QuadTree *sw, const QuadTree *se) {
    QuadTree quadtree = (QuadTree){.depth = depth,
                                   .NW = QUADTREE_VALUE(nw),
                                   .NE = QUADTREE_VALUE(ne),
                                   .SW = QUADTREE_VALUE(sw),
                                   .SE = QUADTREE_VALUE(se)};
    quadtree.hash = hashQuadrants(QUADTREE_VALUE(nw), QUADTREE_VALUE(ne), QUADTREE_VALUE(sw), QUADTREE_VALUE(se));
    quadtree.result = NULL;
    return quadtree;
}

static bool isTreeNode(QuadrantValue qvalue) { return IS_QUADTREE(qvalue); }

// Returns the interned node with the given values. Creates the interned node if it does not exist.
static QuadTree *node(int depth, QuadrantValue nw, QuadrantValue ne, QuadrantValue sw, QuadrantValue se) {
    QuadTree quadtree = (QuadTree){.depth = depth, .NW = nw, .NE = ne, .SW = sw, .SE = se};
    quadtree.hash = hashQuadrants(nw, ne, sw, se);
    quadtree.result = NULL;

    return copyQuadTree(&quadtree);
}

// Creates a quadtree with a constant value throughout
static QuadTree *newConstantQuadTree(int depth, int x) {
    QuadTree baseLeaf = leafNode(x, x, x, x);

    // Check if the empty leaf is already interned
    QuadTree *quadtree = copyQuadTree(&baseLeaf);

    // Building the empty nodes from the leaf node upwards until the tree is full
    for (int i = 2; i <= depth; i++) {
        QuadTree newTree = treeNode(i, quadtree, quadtree, quadtree, quadtree);
        quadtree = copyQuadTree(&newTree);
    }

    return quadtree;
}

// We will set 0 to be the lowest depth (leafs)
QuadTree *newEmptyQuadTree(int depth) { return newConstantQuadTree(depth, 0); }

static Vector2 centerOfQuadrant(Quadrant quadrant, Vector2 center, float width) {
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

// Set's leaf value at the given point in space to the given value, and returns the pointer with that value.
// This doesn't edit the quadtree, just returns the quadtree with that pointer as value
QuadTree *setPointInQuadTree(Vector2 point, Vector2 center, float width, const QuadTree *quadtree,
                             QuadrantValue value) {
    // Find the quadrant the point is located in
    Quadrant quadrant = pointToQuadrant(point, center);
    QuadrantValue qvalue = quadrantGet(quadrant, quadtree);

    QuadTree copy;
    if (isLeaf(qvalue)) {
        // Base Case - This quadtree is a leaf node, and qvalue is an integer
        copy = *quadtree;
        QuadrantValue newValue = AS_INT(value) == -1 ? flip(quadrantGet(quadrant, &copy)) : value;

        // TODO: We use -1 to represent a flip. Integer overflow will cause a problem?
        quadrantSet(quadrant, &copy, newValue);
    } else {
        // Recursively go down until at a leaf
        center = centerOfQuadrant(quadrant, center, width / 2.0f);
        width /= 2;

        QuadTree *subTree = setPointInQuadTree(point, center, width, AS_QUADTREE(qvalue), value);
        copy = *quadtree;
        quadrantSet(quadrant, &copy, QUADTREE_VALUE(subTree));
    }

    // Need to rehash as the quadtree has been altered
    copy.hash = hashQuadTree(&copy);

    return copyQuadTree(&copy);
}

// Drawing

// DRAWING
static void drawInt(int i, int x, int y, float width, float height) {
    drawCenteredSquare((Vector2){x, y}, width * 1.5f, i == 0 ? BLACK : WHITE);
}

static void drawFluid(FluidValue fluid, int x, int y, float width, float height) {
    Color color = ColorBrightness(BLUE, 0.5f - fluid.state / 32.0f);
    drawCenteredSquare((Vector2){x, y}, 2 * width, color);
    DrawText(TextFormat("%d", fluid.state), x, y, 16, WHITE);
}

static void drawQuadrantValue(QuadrantValue qvalue, int x, int y, float width, float height);

static void drawTree(QuadTree *quadtree, int x, int y, float width, float height) {
    Vector2 center = centerOfQuadrant(NW, (Vector2){x, y}, width);
    drawQuadrantValue(quadtree->NW, center.x, center.y, width / 2.0f, height / 2.0f);

    center = centerOfQuadrant(NE, (Vector2){x, y}, width);
    drawQuadrantValue(quadtree->NE, center.x, center.y, width / 2.0f, height / 2.0f);

    center = centerOfQuadrant(SW, (Vector2){x, y}, width);
    drawQuadrantValue(quadtree->SW, center.x, center.y, width / 2.0f, height / 2.0f);

    center = centerOfQuadrant(SE, (Vector2){x, y}, width);
    drawQuadrantValue(quadtree->SE, center.x, center.y, width / 2.0f, height / 2.0f);
}

static void drawQuadrantValue(QuadrantValue qvalue, int x, int y, float width, float height) {
    switch (qvalue.type) {
    case VAL_INT:
        drawInt(AS_INT(qvalue), x, y, width, height);
        break;
    case VAL_FLUID:
        drawFluid(AS_FLUID(qvalue), x, y, width, height);
        break;
    case VAL_TREE:
        drawTree(AS_QUADTREE(qvalue), x, y, width, height);
        break;
    case VAL_EMPTY:
        break;
    }
}

void drawQuadTree(QuadTree quadtree, Vector2 center, float width, Camera2D camera) {
    drawTree(&quadtree, center.x, center.y, width / 2.0f, width / 2.0f);
}

void drawQuadTreeOld(QuadTree quadtree, Vector2 center, float width, Camera2D camera) {

#define DRAW_QUAD(tree, quad)                                                                                          \
    (drawQuadTree(*AS_QUADTREE(tree.quad), centerOfQuadrant(quad, center, width / 2.0f), width / 2.0f, camera))
#define DRAW_INT(tree, quad)                                                                                           \
    (drawCenteredSquare(centerOfQuadrant(quad, center, width / 2.0f), 0.9f * width / 2.0f,                             \
                        AS_INT(tree.quad) == 0 ? BLACK : BLUE))

    if (isTreeNode(quadtree.NW)) {
        DRAW_QUAD(quadtree, NW);
        DRAW_QUAD(quadtree, NE);
        DRAW_QUAD(quadtree, SW);
        DRAW_QUAD(quadtree, SE);

#ifdef DEBUG_QUADINFO
        Vector2 textPos = centerOfQuadrant(NW, center, width / 2.0f);
        DrawText(TextFormat("%p \n %lu", AS_QUADTREE(quadtree.NW), AS_QUADTREE(quadtree.NW)->hash), textPos.x,
                 textPos.y, 12, WHITE);
        textPos = centerOfQuadrant(NE, center, width / 2.0f);
        DrawText(TextFormat("%p \n %lu", AS_QUADTREE(quadtree.NE), AS_QUADTREE(quadtree.NE)->hash), textPos.x,
                 textPos.y, 12, WHITE);
        textPos = centerOfQuadrant(SW, center, width / 2.0f);
        DrawText(TextFormat("%p \n %lu", AS_QUADTREE(quadtree.SW), AS_QUADTREE(quadtree.SW)->hash), textPos.x,
                 textPos.y, 12, WHITE);
        textPos = centerOfQuadrant(SE, center, width / 2.0f);
        DrawText(TextFormat("%p \n %lu", AS_QUADTREE(quadtree.SE), AS_QUADTREE(quadtree.SE)->hash), textPos.x,
                 textPos.y, 12, WHITE);
#endif
    } else if (isLeaf(quadtree.NW)) {
        DRAW_INT(quadtree, NW);
        DRAW_INT(quadtree, NE);
        DRAW_INT(quadtree, SW);
        DRAW_INT(quadtree, SE);
    }

#ifdef DEBUG_DRAW_QUADS
    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
    drawCenteredSquare(center, 2, quadtree.depth % 2 == 0 ? GREEN : PURPLE);
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

int maxQuads(const QuadTree *quadtree) { return pow(2, quadtree->depth); }

float miniumumQuadSize(float width, const QuadTree *quadtree) { return width / (maxQuads(quadtree)); }

// Game of Life
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

CellNeighbourhood fromQuadrantValues(QuadrantValue nw, QuadrantValue n, QuadrantValue ne, QuadrantValue w,
                                     QuadrantValue c, QuadrantValue e, QuadrantValue sw, QuadrantValue s,
                                     QuadrantValue se) {
    return (CellNeighbourhood){nw, n, ne, w, c, e, sw, s, se};
}

static int surroundingSum(CellNeighbourhood n) {
    return AS_INT(n.nw) + AS_INT(n.n) + AS_INT(n.ne) + AS_INT(n.w) + AS_INT(n.e) + AS_INT(n.sw) + AS_INT(n.s) +
           AS_INT(n.se);
}

static int gameOfLife(CellNeighbourhood n) {
    int count = surroundingSum(n);
    if (AS_INT(n.c) == 0) {
        // Dead cell
        return count == 3;
    }
    // Live cell
    return count == 2 || count == 3;
}

static bool canAbsorb(QuadrantValue qvalue) {
    bool isFillableFluid = IS_FLUID(qvalue) && AS_FLUID(qvalue).state < 16;
    return isFillableFluid || isEmpty(qvalue);
}

static bool canFlow(QuadrantValue qvalue) { return IS_FLUID(qvalue); }

// Returns true if `destination` has less fluid than `source`.
// <!> Assumes two fluid values as input
static bool hasLessLiquid(QuadrantValue source, QuadrantValue destination) {
    if (!IS_FLUID(destination)) {
        return false;
    }

    return AS_FLUID(destination).state < AS_FLUID(source).state;
}

// Whether or not the fluid in `source` will fall into `destination`
static bool willFall(QuadrantValue source, QuadrantValue destination) {
    if (isEmpty(destination)) {
        return true;
    }
    if (!IS_FLUID(destination)) {
        return false;
    }
    return AS_FLUID(destination).state < AS_FLUID(source).state;
    return hasLessLiquid(source, destination);
}

// Returns the fluid value for the source after flowing to destination
static QuadrantValue spreadFrom(QuadrantValue source, QuadrantValue destination) {
    FluidValue sourceFluid = AS_FLUID(source);
    FluidValue destFluid = AS_FLUID(destination);

    int maxFlowable = 16 - destFluid.state;

    int newAmount = sourceFluid.state < maxFlowable ? 0 : sourceFluid.state - maxFlowable;
    if (newAmount == 0) {
        return INT_VALUE(0);
    }
    FluidType type = AS_FLUID(destination).type;

    FluidValue fvalue = (FluidValue){type, newAmount};
    return FLUID_VALUE(fvalue);
}

static QuadrantValue doubleSidewaysFlow(QuadrantValue source) {
    int newState = AS_FLUID(source).state / 2;
    if (newState == 0) {
        return INT_VALUE(0);
    }

    FluidType type = AS_FLUID(source).type;
    FluidValue fvalue = (FluidValue){type, newState};

    return FLUID_VALUE(fvalue);
}

static int singleSidewaysFlow(QuadrantValue source, QuadrantValue destination) {
    FluidValue sourceFluid = AS_FLUID(source);

    int difference;
    if (IS_FLUID(destination)) {
        difference = sourceFluid.state - AS_FLUID(destination).state;
    } else if (isEmpty(destination)) {
        difference = sourceFluid.state;
    } else {
        return 0;
    }

    if (difference > 0) {
        return difference / 2;
    }
    return 0;
}

static int singleSidewaysAbsorb(QuadrantValue source, QuadrantValue destination) {
    if (!IS_FLUID(source)) {
        return 0;
    }

    int destState = IS_FLUID(destination) ? AS_FLUID(destination).state : 0;
    int difference = AS_FLUID(source).state - destState;

    if (difference > 0) {
        return difference / 2;
    }
    return 0;
}

static QuadrantValue spread(CellNeighbourhood n) {
    if (!IS_FLUID(n.c)) {
        return n.c;
    }

    if (canAbsorb(n.s)) {
        if (isEmpty(n.s)) {
            // Fluid flows into empty spot.
            return INT_VALUE(0);
        }

        if (willFall(n.c, n.s)) {
            return spreadFrom(n.c, n.s);
        }
    }

    if (AS_FLUID(n.c).state == 1) {
        return n.c;
    }

    int leftLoss = canAbsorb(n.w) ? singleSidewaysFlow(n.c, n.w) : 0;
    int rightLoss = canAbsorb(n.e) ? singleSidewaysFlow(n.c, n.e) : 0;
    int newState = AS_FLUID(n.c).state - leftLoss - rightLoss;

    if (newState == 0) {
        return INT_VALUE(0);
    } else {
        FluidValue fvalue = (FluidValue){AS_FLUID(n.c).type, newState};
        return FLUID_VALUE(fvalue);
    }
}

// Returns the fluid value for the destination after flowing from the source
static QuadrantValue flowFrom(QuadrantValue source, QuadrantValue destination) {
    FluidValue sourceFluid = AS_FLUID(source);
    FluidValue destFluid = AS_FLUID(destination);

    int maxFlowable = 16 - destFluid.state;

    int flowAmount = sourceFluid.state < maxFlowable ? sourceFluid.state : maxFlowable;
    FluidType type = AS_FLUID(destination).type;

    FluidValue fvalue = (FluidValue){type, destFluid.state + flowAmount};
    return FLUID_VALUE(fvalue);
}

static QuadrantValue absorbSideways(QuadrantValue destination) {
    FluidValue fvalue;
    if (IS_FLUID(destination)) {
        fvalue = (FluidValue){AS_FLUID(destination).type, AS_FLUID(destination).state + 2};
    } else {
        fvalue = (FluidValue){FLUID_WATER, 2};
    }

    return FLUID_VALUE(fvalue);
}

static QuadrantValue absorbSidewaysTwice(QuadrantValue destination) {
    FluidValue fvalue;
    if (IS_FLUID(destination)) {
        fvalue = (FluidValue){AS_FLUID(destination).type, AS_FLUID(destination).state + 4};
    } else {
        fvalue = (FluidValue){FLUID_WATER, 4};
    }

    return FLUID_VALUE(fvalue);
}

static QuadrantValue absorb(CellNeighbourhood n) {
    if (canAbsorb(n.c)) {
        // Northern cell - If fluid exists in the northern cell, it will try to fill up the lower cell
        if (IS_FLUID(n.n)) {
            if (isEmpty(n.c)) {
                // Fluid falling into empty cell
                return n.n;
            }

            if (willFall(n.n, n.c)) {
                return flowFrom(n.n, n.c);
            }
        }

        int leftGain = IS_FLUID(n.w) && willFall(n.w, n.sw) == false ? singleSidewaysAbsorb(n.w, n.c) : 0;
        int rightGain = IS_FLUID(n.e) && willFall(n.e, n.se) == false ? singleSidewaysAbsorb(n.e, n.c) : 0;
        int gain = leftGain + rightGain;

        FluidValue fvalue;
        if (IS_FLUID(n.c)) {
            fvalue = (FluidValue){AS_FLUID(n.c).type, AS_FLUID(n.c).state + gain};
        } else {
            // Empty value.
            if (gain == 0) {
                return INT_VALUE(0);
            }
            fvalue = (FluidValue){FLUID_WATER, gain};
        }
        return FLUID_VALUE(fvalue);
    }

    return n.c;
}

static QuadrantValue fluid(CellNeighbourhood n) {
    if (isEmpty(n.c)) {
        return absorb(n);
    }
    if (IS_FLUID(n.c)) {
        QuadrantValue afterSpread = spread(n);
        QuadrantValue afterAbsorb = absorb(n);

        if (isEmpty(afterSpread)) {
            if (AS_FLUID(afterAbsorb).state == AS_FLUID(n.c).state) {
                return INT_VALUE(0);
            }

            int amount = AS_FLUID(afterAbsorb).state - AS_FLUID(n.c).state;
            if (amount < 0)
                LogMessage(LOG_ERROR, "1: Fluid amount negative! %d", amount);

            FluidValue result = (FluidValue){AS_FLUID(n.c).type, amount};

            return FLUID_VALUE(result);
        }

        int amount = AS_FLUID(afterSpread).state + AS_FLUID(afterAbsorb).state - AS_FLUID(n.c).state;
        if (amount < 0)
            LogMessage(LOG_ERROR, "2: Fluid amount negative! (%d + %d - %d = %d)", AS_FLUID(afterSpread).state,
                       AS_FLUID(afterAbsorb).state, AS_FLUID(n.c).state, amount);
        FluidValue combined = (FluidValue){AS_FLUID(afterSpread).type, amount};

        return FLUID_VALUE(combined);
    }

    return n.c;
}

static QuadrantValue identity(CellNeighbourhood n) { return n.c; }

static int sand(CellNeighbourhood n) {
    if (AS_INT(n.c) == 1) {
        if (AS_INT(n.s) == 0) {
            return 0;
        }
        if (AS_INT(n.sw) == 0 && AS_INT(n.w) == 0) {
            return 0;
        }
        if (AS_INT(n.se) == 0 && AS_INT(n.e) == 0) {
            return 0;
        }
    }
    if (AS_INT(n.c) == 0) {
        if (AS_INT(n.n) == 1) {
            return 1;
        }
        if (AS_INT(n.nw) == 1 && AS_INT(n.w) != 0) {
            return 1;
        }
        if (AS_INT(n.ne) == 1 && AS_INT(n.e) != 0) {
            return 1;
        }
    }
    return AS_INT(n.c);
}

static QuadTree *evolveBaseCase(QuadTree *quadtree) {
    QuadTree *nw = AS_QUADTREE(quadtree->NW);
    QuadTree *ne = AS_QUADTREE(quadtree->NE);
    QuadTree *sw = AS_QUADTREE(quadtree->SW);
    QuadTree *se = AS_QUADTREE(quadtree->SE);

    QuadrantValue (*f)(CellNeighbourhood n) = fluid;

    CellNeighbourhood n = fromQuadrantValues(nw->NW, nw->NE, ne->NW, nw->SW, nw->SE, ne->SW, sw->NW, sw->NE, se->NW);
    QuadrantValue center_nw = f(n);

    n = fromQuadrantValues(nw->NE, ne->NW, ne->NE, nw->SE, ne->SW, ne->SE, sw->NE, se->NW, se->NE);
    QuadrantValue center_ne = f(n);

    n = fromQuadrantValues(nw->SW, nw->SE, ne->SW, sw->NW, sw->NE, se->NW, sw->SW, sw->SE, se->SW);
    QuadrantValue center_sw = f(n);

    n = fromQuadrantValues(nw->SE, ne->SW, ne->SE, sw->NE, se->NW, se->NE, sw->SE, se->SW, se->SE);
    QuadrantValue center_se = f(n);

    QuadTree *result = node(0, center_nw, center_ne, center_sw, center_se);

    quadtree->result = result;

    return result;
}

// Returns a quadtree with a depth 1 lower than the given tree
static QuadTree *evolve(QuadTree *quadtree) {
    if (quadtree->result != NULL) {
        return quadtree->result;
    }

    if (quadtree->depth == 2) {
        return evolveBaseCase(quadtree);
    }

    QuadTree *nw = AS_QUADTREE(quadtree->NW);
    QuadTree *ne = AS_QUADTREE(quadtree->NE);
    QuadTree *sw = AS_QUADTREE(quadtree->SW);
    QuadTree *se = AS_QUADTREE(quadtree->SE);

    QuadTree *nw_center = evolve(AS_QUADTREE(quadtree->NW));
    QuadTree *ne_center = evolve(AS_QUADTREE(quadtree->NE));
    QuadTree *sw_center = evolve(AS_QUADTREE(quadtree->SW));
    QuadTree *se_center = evolve(AS_QUADTREE(quadtree->SE));

    QuadTree *n = evolve(&(QuadTree){
        .depth = quadtree->depth - 1,
        .NW = nw->NE,
        .NE = ne->NW,
        .SW = nw->SE,
        .SE = ne->SW,
    });

    QuadTree *e = evolve(&(QuadTree){
        .depth = quadtree->depth - 1,
        .NW = ne->SW,
        .NE = ne->SE,
        .SW = se->NW,
        .SE = se->NE,
    });

    QuadTree *s = evolve(&(QuadTree){
        .depth = quadtree->depth - 1,
        .NW = sw->NE,
        .NE = se->NW,
        .SW = sw->SE,
        .SE = se->SW,
    });

    QuadTree *w = evolve(&(QuadTree){
        .depth = quadtree->depth - 1,
        .NW = nw->SW,
        .NE = nw->SE,
        .SW = sw->NW,
        .SE = sw->NE,
    });

    QuadTree *c = evolve(&(QuadTree){
        .depth = quadtree->depth - 1,
        .NW = nw->SE,
        .NE = ne->SW,
        .SW = sw->NE,
        .SE = se->NW,
    });

    QuadTree *result_nw = node(quadtree->depth - 2, nw_center->SE, n->SW, w->NE, c->NW);
    QuadTree *result_ne = node(quadtree->depth - 2, n->SE, ne_center->SW, c->NE, e->NW);
    QuadTree *result_sw = node(quadtree->depth - 2, w->SE, c->SW, sw_center->NE, s->NW);
    QuadTree *result_se = node(quadtree->depth - 2, c->SE, e->SW, s->NE, se_center->NW);

    QuadTree result = treeNode(quadtree->depth - 1, result_nw, result_ne, result_sw, result_se);

    QuadTree *interned = copyQuadTree(&result);
    quadtree->result = interned;

    return interned;
}

QuadTree *evolveQuadtree(const QuadTree *quadtree) {
    // TODO: Improve this by not recreating the empty each time - Possible store the quadtree in a 1 up date structure
    // and work with that!

    // Game of life
    QuadTree *empty = newConstantQuadTree(quadtree->depth - 1, -1);

    // Testing
    // QuadTree *empty = newConstantQuadTree(quadtree->depth - 1, -1);

    QuadTree nw = treeNode(quadtree->depth, empty, empty, empty, AS_QUADTREE(quadtree->NW));
    QuadTree ne = treeNode(quadtree->depth, empty, empty, AS_QUADTREE(quadtree->NE), empty);
    QuadTree sw = treeNode(quadtree->depth, empty, AS_QUADTREE(quadtree->SW), empty, empty);
    QuadTree se = treeNode(quadtree->depth, AS_QUADTREE(quadtree->SE), empty, empty, empty);

    QuadTree wrapped = treeNode(quadtree->depth + 1, &nw, &ne, &sw, &se);

    // DONT DO THIS! You should never edit the contents of an interned object
    // AS_QUADTREE(wrapper->NW)->SE = quadtree->NW;
    // AS_QUADTREE(wrapper->NE)->SW = quadtree->NE;
    // AS_QUADTREE(wrapper->SW)->NE = quadtree->SW;
    // AS_QUADTREE(wrapper->SE)->NW = quadtree->SE;

    return evolve(&wrapped);
}
