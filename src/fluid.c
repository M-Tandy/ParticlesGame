#include "fluid.h"
#include "common.h"
#include "quadtree.h"
#include <assert.h>

#include "debug.h"
#include <raylib.h>

// Currently unnused!
#define MAX_FLUID_STATE 32

static QuadrantValue identity(CellNeighbourhood n) { return n.c; }

static void initOccupationNumber(OccupationNumber *occ) {
    occ->nw = 0;
    occ->n = 0;
    occ->ne = 0;
    occ->w = 0;
    occ->c = 0;
    occ->e = 0;
    occ->sw = 0;
    occ->s = 0;
    occ->se = 0;
}

// TODO: In quadtree.h also. Simplify
static CellNeighbourhood fromQuadrantValues(QuadrantValue nw, QuadrantValue n, QuadrantValue ne, QuadrantValue w,
                                            QuadrantValue c, QuadrantValue e, QuadrantValue sw, QuadrantValue s,
                                            QuadrantValue se) {
    return (CellNeighbourhood){nw, n, ne, w, c, e, sw, s, se};
}

static Fluid newFluid(Type type, int state) { return (Fluid){.type = type, .state = state}; }

static FluidValue emptyFluid(FluidType type) { return (FluidValue){type, 0}; }

// Resoles a qvalue to a fluid. If the qvalue is not a fluid type, returns empty water.
static FluidValue resolve(QuadrantValue qvalue) {
    if (IS_FLUID(qvalue)) {
        return AS_FLUID(qvalue);
    }
    if (!IS_INT(qvalue) || AS_INT(qvalue) != 0) {
        // Non empty cell.
        // TODO: Improve error message.
        LogMessage(LOG_ERROR, "Failed to resolve fluid.");
    }

    // Empty cell. We gracefully assume that the cell is empty upon error.
    return (FluidValue){FLUID_WATER, 0};
}

// Returns the difference in states between two fluids.
static int difference(FluidValue left, FluidValue right) { return left.state - right.state; }

static bool canFlow(QuadrantValue source) { return IS_FLUID(source) && AS_FLUID(source).state > 0; }

static bool canFallTo(QuadrantValue source, QuadrantValue destination) {
    if (canFlow(source)) {
        if (IS_INT(destination) && AS_INT(destination) == 0)
            return true;
        if (IS_FLUID(destination)) {
            return AS_FLUID(destination).state < MAX_FLUID_STATE;
        }
    }
    return false;
}

static bool canFlowTo(QuadrantValue source, QuadrantValue destination) {
    if (canFlow(source)) {
        if (IS_INT(destination) && AS_INT(destination) == 0)
            return true;
        if (IS_FLUID(destination)) {
            return AS_FLUID(destination).state < AS_FLUID(source).state;
        }
    }

    return false;
}

// Tries to flow amount `n` of the `source` into `destination`.
static void nFlow(uint32_t n, QuadrantValue source, QuadrantValue destination, QuadrantValue *sourceAfter,
                  QuadrantValue *destAfter) {
    FluidValue sourceFluid = resolve(source);
    FluidValue destFluid = resolve(destination);

    sourceFluid.state -= n;

    if (sourceAfter != NULL) {
        *sourceAfter = sourceFluid.state <= 0 ? INT_VALUE(0) : FLUID_VALUE(sourceFluid);
    }

    if (destAfter != NULL) {
        FluidValue destResult;
        if (IS_FLUID(destination)) {
            FluidValue destResult = AS_FLUID(destination);
            destResult.state += n;
        } else {
            destResult = (FluidValue){sourceFluid.type, n};
        }
        *destAfter = FLUID_VALUE(destResult);
    }
}

// Tries to store the result of flowing from the source to the destination in sourceAfter and destAfter
static void fullFlow(QuadrantValue source, QuadrantValue destination, QuadrantValue *sourceAfter,
                     QuadrantValue *destAfter) {
    FluidValue sourceFluid = resolve(source);
    FluidValue destFluid = resolve(destination);

    nFlow(difference(sourceFluid, destFluid), source, destination, sourceAfter, destAfter);
}

// Attempts to spread the central elements fluid to the two neighbouring cells.
static QuadrantValue spread(CellNeighbourhood n) {
    int loss = 0;
    if (canFlowTo(n.c, n.w)) {
        loss += difference(resolve(n.c), resolve(n.w)) / 2;
    }
    if (canFlowTo(n.c, n.e)) {
        loss += difference(resolve(n.c), resolve(n.e)) / 2;
    }
    if (loss > 0) {
        FluidValue source = resolve(n.c);
        source.state -= loss;
        return FLUID_VALUE(source);
    }

    return identity(n);
}

// Tries to flow the fluid from the center cell to the southern cell. Defaults to `spread` if incapable.
static QuadrantValue fall(CellNeighbourhood n) {
    QuadrantValue result;
    if (canFlowTo(n.c, n.s)) {
        fullFlow(n.c, n.s, &result, NULL);
        return result;
    };
    return spread(n);
}

// Returns the ammount of fluid after absorbtion.
static int absorb(CellNeighbourhood n) {
    QuadrantValue result;

    if (canFlowTo(n.n, n.c)) {
        // Gravity of fluid from above.
        return difference(resolve(n.n), resolve(n.c));
    }

    int gain = 0;
    if (!canFlowTo(n.w, n.sw) && canFlowTo(n.w, n.c)) {
        gain += difference(resolve(n.w), resolve(n.c)) / 2;
    }
    if (!canFlowTo(n.e, n.se) && canFlowTo(n.e, n.c)) {
        gain += difference(resolve(n.e), resolve(n.c)) / 2;
    }

    if (gain != 0) {
        return gain;
    }

    return 0;
}

static bool isOverPressurised(QuadrantValue source, QuadrantValue destination) {
    return AS_FLUID(source).state > MAX_FLUID_STATE;
}

static int resolvePressureDifference(QuadrantValue source, QuadrantValue destination) {
    return AS_FLUID(source).state - MAX_FLUID_STATE;
}

#define MAX_FLOW 1
// Restrict the flow amount by bounding it below the max flow ammount
static int constrain(int x) { return x <= MAX_FLOW ? x : MAX_FLOW; }

// Assumes center element is a fluid
static OccupationNumber collision(CellNeighbourhood n) {
    OccupationNumber occ;
    initOccupationNumber(&occ);
    occ.c = resolve(n.c).state;
    
    int diff = 0;
    if (canFallTo(n.c, n.s)) {
        int maxFlow = MAX_FLUID_STATE - resolve(n.s).state;
        diff = difference(resolve(n.c), resolve(n.s));
        occ.s = diff > 0 ? diff : -diff;
        occ.s = occ.s > maxFlow ? maxFlow : occ.s;
        occ.c -= occ.s;
        AS_FLUID(n.c).state -= occ.s;
    }
    if (canFallTo(n.c, n.sw)) {
        int maxFlow = MAX_FLUID_STATE - resolve(n.sw).state;
        diff = difference(resolve(n.c), resolve(n.sw));
        occ.sw = diff > 0 ? diff : -diff;
        occ.sw = occ.sw > maxFlow ? maxFlow : occ.sw;
        occ.c -= occ.sw;
        AS_FLUID(n.c).state -= occ.sw;
    }
    if (canFallTo(n.c, n.se)) {
        int maxFlow = MAX_FLUID_STATE - resolve(n.se).state;
        diff = difference(resolve(n.c), resolve(n.se));
        occ.se = diff > 0 ? diff : -diff;
        occ.se = occ.sw > maxFlow ? maxFlow : occ.sw;
        occ.c -= occ.se;
        AS_FLUID(n.c).state -= occ.se;
    }
    if (canFlowTo(n.c, n.w)) {
        diff = difference(resolve(n.c), resolve(n.w));
        occ.w = constrain(diff / 2);
        occ.c -= occ.w;
        AS_FLUID(n.c).state -= occ.w;
    }
    if (canFlowTo(n.c, n.e)) {
        diff = difference(resolve(n.c), resolve(n.e));
        occ.e = constrain(diff / 2);
        occ.c -= occ.e;
        AS_FLUID(n.c).state -= occ.e;
    }
    if (canFlowTo(n.c, n.n) && isOverPressurised(n.c, n.n)) {
        occ.n = resolvePressureDifference(n.c, n.n);
        occ.c -= occ.n;
    }

    if (resolve(n.c).state < occ.c) {
        LogMessage(LOG_ERROR, "Center state grew: %d -> %d", resolve(n.c).state, occ.c);
    }

    return occ;
}

static int surroundingSum(CellNeighbourhood n) { // clang-format off
    int nw    = IS_OCCUPATION_NUMBER(n.nw) ? AS_OCCUPATION_NUMBER(n.nw).se : 0;
    int north = IS_OCCUPATION_NUMBER(n.n)  ? AS_OCCUPATION_NUMBER(n.n).s   : 0;
    int ne    = IS_OCCUPATION_NUMBER(n.ne) ? AS_OCCUPATION_NUMBER(n.ne).sw : 0;
    int w     = IS_OCCUPATION_NUMBER(n.w)  ? AS_OCCUPATION_NUMBER(n.w).e   : 0;
    int e     = IS_OCCUPATION_NUMBER(n.e)  ? AS_OCCUPATION_NUMBER(n.e).w   : 0;
    int sw    = IS_OCCUPATION_NUMBER(n.sw) ? AS_OCCUPATION_NUMBER(n.sw).ne : 0;
    int s     = IS_OCCUPATION_NUMBER(n.s)  ? AS_OCCUPATION_NUMBER(n.s).n   : 0;
    int se    = IS_OCCUPATION_NUMBER(n.se) ? AS_OCCUPATION_NUMBER(n.se).nw : 0; // clang-format on

    return nw + north + ne + w + e + sw + s + se;
}

// Resolve a neighbourhood with a fluid in the middle. Assumes `n.c` is a fluid.
QuadrantValue fluidNeighbourhood(CellNeighbourhood n) {
    if (canFlow(n.c)) {
        return OCCUPATION_NUMBER_VALUE(collision(n));
    }
    if (IS_OCCUPATION_NUMBER(n.c) || (IS_INT(n.c) && AS_INT(n.c) == 0)) {
        int center = IS_OCCUPATION_NUMBER(n.c) ? AS_OCCUPATION_NUMBER(n.c).c : 0;
        FluidValue fvalue = (FluidValue){FLUID_WATER, surroundingSum(n) + center};
        if (fvalue.state > 0) {
            return FLUID_VALUE(fvalue);
        } else {
            return INT_VALUE(0);
        }
    }

    return n.c;
    // QuadrantValue loss = canFlow(n.c) ? fall(n) : FLUID_VALUE(emptyFluid(FLUID_WATER));
    // int absorbAmount = absorb(n);
    //
    // FluidValue result = resolve(loss);
    // result.state += absorbAmount;
    //
    // if (result.state == 0) {
    //     return INT_VALUE(0);
    // } else {
    //     return FLUID_VALUE(result);
    // }
}

#undef MAX_FLUID_STATE
