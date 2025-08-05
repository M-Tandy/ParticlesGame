
#include "neighbourhood.h"
#include "debug.h"
#include "value.h"

CellNeighbourhood newCellNeighbourhood(CellValue *nw, CellValue *n, CellValue *ne, CellValue *w, CellValue *c,
                                       CellValue *e, CellValue *sw, CellValue *s, CellValue *se) {
    return (CellNeighbourhood){// clang-format off
        .nw = nw,
        .n = n,
        .ne = ne,
        .w = w,
        .c = c,
        .e = e,
        .sw = sw,
        .s = s,
        .se = se
    }; // clang-format on
}

static bool canFlow(CellValue source) { return isFluid(source) && source.state > 0; }

#define MAX_FLUID_STATE 64
#define MAX_FLOW 4
static bool canFallTo(CellValue source, CellValue destination) {
    if (canFlow(source)) {
        if (isEmpty(destination))
            return true;
        if (isFluid(destination)) {
            return source.material == destination.material && destination.state < MAX_FLUID_STATE;
        }
    }
    return false;
}

static bool canFlowTo(CellValue source, CellValue destination) {
    if (canFlow(source)) {
        if (isEmpty(destination))
            return true;
        if (isFluid(destination)) {
            return source.material == destination.material && destination.state < source.state;
        }
    }

    return false;
}

static int constrain(int x) { return x <= MAX_FLOW ? x : MAX_FLOW; }

static bool isOverPressurised(CellValue source, CellValue destination) { return source.state > MAX_FLUID_STATE; }

static int resolvePressureDifference(CellValue source, CellValue destination) { return source.state - MAX_FLUID_STATE; }

int surroundingSum(CellNeighbourhood n) {
    return n.nw->occ.se + n.n->occ.s + n.ne->occ.sw + n.w->occ.e + +n.c->occ.c + n.e->occ.w + n.sw->occ.ne +
           n.s->occ.n + n.se->occ.nw;
}

CMaterial determineMaterial(CellNeighbourhood n) {
    if (n.nw->occ.se != 0)
        return n.nw->material;
    if (n.n->occ.s != 0)
        return n.n->material;
    if (n.ne->occ.sw != 0)
        return n.ne->material;
    if (n.w->occ.e != 0)
        return n.w->material;
    if (n.c->occ.c != 0)
        return n.c->material;
    if (n.e->occ.w != 0)
        return n.e->material;
    if (n.sw->occ.ne != 0)
        return n.sw->material;
    if (n.s->occ.n != 0)
        return n.s->material;
    if (n.se->occ.nw != 0)
        return n.se->material;

    return NONE;
}

OccupationNumber collide(CellNeighbourhood n) {
    OccupationNumber occ;
    initOccupationNumber(&occ);
    int originalState = n.c->state;
    if (n.c->type != FLUID) {
        return occ;
    }
    occ.c = n.c->state;

    int diff = 0;
    if (canFallTo(*n.c, *n.s)) {
        int maxFlow = MAX_FLUID_STATE - n.s->state;
        maxFlow = maxFlow < n.c->state ? maxFlow : n.c->state;
        diff = difference(*n.c, *n.s);
        occ.s = diff > 0 ? diff : -diff;
        occ.s = occ.s > maxFlow ? maxFlow : occ.s;
        occ.c -= occ.s;
        n.c->state -= occ.s;
    }
    if (canFallTo(*n.c, *n.sw)) {
        int maxFlow = MAX_FLUID_STATE - n.sw->state;
        maxFlow = maxFlow < n.c->state ? maxFlow : n.c->state;
        diff = difference(*n.c, *n.sw);
        occ.sw = diff > 0 ? diff : -diff;
        occ.sw = occ.sw > maxFlow ? maxFlow : occ.sw;
        occ.c -= occ.sw;
        n.c->state -= occ.sw;
    }
    if (canFallTo(*n.c, *n.se)) {
        int maxFlow = MAX_FLUID_STATE - n.se->state;
        maxFlow = maxFlow < n.c->state ? maxFlow : n.c->state;
        diff = difference(*n.c, *n.se);
        occ.se = diff > 0 ? diff : -diff;
        occ.se = occ.sw > maxFlow ? maxFlow : occ.sw;
        occ.c -= occ.se;
        n.c->state -= occ.se;
    }
    if (canFlowTo(*n.c, *n.w)) {
        diff = difference(*n.c, *n.w);
        occ.w = constrain(diff / 2);
        occ.c -= occ.w;
        n.c->state -= occ.w;
    }
    if (canFlowTo(*n.c, *n.e)) {
        diff = difference(*n.c, *n.e);
        occ.e = constrain(diff / 2);
        occ.c -= occ.e;
        n.c->state -= occ.e;
    }
    if (canFlowTo(*n.c, *n.n) && isOverPressurised(*n.c, *n.n)) {
        occ.n = resolvePressureDifference(*n.c, *n.n);
        occ.c -= occ.n;
    }

    if (n.c->state < occ.c) {
        LogMessage(LOG_ERROR, "Center state grew: %d -> %d", n.c->state, occ.c);
    }

    n.c->state = originalState;

    // LogMessage(LOG_INFO, "OccupationNumber calculated to: nw: %d, n : %d, ne : %d, w : %d, c : %d, e : %d, sw : %d, s
    // : %d, se : %d", occ.nw, occ.n, occ.ne, occ.w, occ.c, occ.e, occ.sw, occ.s, occ.se);

    return occ;
}

static bool materialInSurrounding(CellNeighbourhood n, CMaterial material) {
    return n.nw->material == material || n.n->material == material || n.ne->material == material ||
           n.w->material == material || n.e->material == material || n.sw->material == material ||
           n.s->material == material || n.se->material == material;
}

typedef struct Reaction {
    CMaterial reactant;
    CType type;
    CMaterial result;
} Reaction;

// clang-format off
const Reaction reactions[] = {
    [NONE]  = { NONE,  VACUUM, NONE },
    [AIR]   = { NONE,  VACUUM, NONE },
    [WATER] = { NONE,  VACUUM, NONE },
    [LAVA]  = { WATER, SOLID,  STONE },
    [STONE] = { NONE,  VACUUM, NONE },
};
// clang-format on

static bool getReaction(CMaterial material, Reaction *reaction) {
    if (reactions[material].reactant != NONE) {
        *reaction = reactions[material];
        return true;
    }
    return false;
}

CellValue react(CellNeighbourhood n) {
    Reaction reaction;
    if (getReaction(n.c->material, &reaction)) {
        if (materialInSurrounding(n, reaction.reactant)) {
            return newCellValue(reaction.type, reaction.result, n.c->state);
        }
    }

    return *n.c;
}
#undef MAX_FLUID_STATE
#undef MAX_FLOW
