#include "value.h"
#include <raylib.h>

void InitOccupationNumber(OccupationNumber *occ) {
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

void InitCell(Cell *cvalue, CType type, CMaterial material, int state) {
    cvalue->type = type;
    cvalue->material = material;
    cvalue->state = state;
    // initOccupationNumber(&cvalue->occ);
    cvalue->settled = false;
}

Cell NewCell(CType type, CMaterial material, int state) {
    Cell cvalue;
    InitCell(&cvalue, type, material, state);
    return cvalue;
}

// clang-format off
static const Color color[] = {
    [NONE]  = NONE,
    [AIR]   = WHITE,
    [WATER] = BLUE,
    [LAVA]  = RED,
    [STONE] = GRAY,
};
// clang-format on

static Color solidColor(CMaterial material) { return color[material]; }

static Color fluidColor(CMaterial material, double state) {
    float brightness = 0.5 - state / 2.0;
    return ColorBrightness(color[material], brightness);
}

Color CellColor(Cell cvalue) {
    if (IS_SOLID(cvalue)) {
        return solidColor(cvalue.material);
    } else if (IS_FLUID(cvalue)) {
        return fluidColor(cvalue.material, cvalue.state);
    }

    return DARKGRAY;
}

void DrawCell(Cell cvalue, int x, int y, int width, int height) {
    Vector2 pos = (Vector2){x, y};
    Color color = CellColor(cvalue);
    DrawRectangle(x, y, width, width, color);
}

void CopyCell(const Cell *source, Cell *destination) {
    destination->type = source->type;
    destination->material = source->material;
    destination->state = source->state;
    // destination->doubleState = source->doubleState;
}

void setCell(Cell *cvalue, int state) {
    if (0 <= state) {
        cvalue->state = state;
    }
}

bool IsEmpty(Cell cvalue) { return cvalue.type == VACUUM; }

bool IsFluid(Cell cvalue) { return cvalue.type == FLUID; }

int Difference(Cell left, Cell right) { return left.state - right.state; }
