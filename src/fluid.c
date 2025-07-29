#include "fluid.h"
#include "common.h"
#include "quadtree.h"

#include "debug.h"
#include <raylib.h>

// Currently unnused!
#define MAX_FLUID_STATE 16

static QuadrantValue identity(CellNeighbourhood n) { return n.c; }

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

// Resolve a neighbourhood with a fluid in the middle. Assumes `n.c` is a fluid.
QuadrantValue fluidNeighbourhood(CellNeighbourhood n) {
    QuadrantValue loss = canFlow(n.c) ? fall(n) : FLUID_VALUE(emptyFluid(FLUID_WATER));
    int absorbAmount = absorb(n);

    FluidValue result = resolve(loss);
    result.state += absorbAmount;

    if (result.state == 0) {
        return INT_VALUE(0);
    } else {
        return FLUID_VALUE(result);
    }
}

#undef MAX_FLUID_STATE
