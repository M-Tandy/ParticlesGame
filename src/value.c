#include "value.h"
#include "cell.h"
#include "draw.h"
#include <raylib.h>

void initOccupationNumber(OccupationNumber *occ) {
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

void initCellValue(CellValue *cvalue, CType type, CMaterial material, int state) {
    cvalue->type = type;
    cvalue->material = material;
    cvalue->state = state;
    initOccupationNumber(&cvalue->occ);
}

CellValue newCellValue(CType type, CMaterial material, int state) {
    CellValue cvalue;
    initCellValue(&cvalue, type, material, state);
    return cvalue;
}

Color cellColor(CellValue cvalue) {
    float brightness = 0.5f - cvalue.state / 64.0f;
    switch (cvalue.material) {
    case WATER:
        return ColorBrightness(BLUE, brightness);
    case LAVA:
        return ColorBrightness(RED, brightness);
    case STONE:
        return ColorBrightness(GRAY, brightness);
    default:
        return DARKGRAY;
    }
}

void drawCellValue(CellValue cvalue, int x, int y, int width, int height) {
    Vector2 pos = (Vector2){x, y};
    if (cvalue.material == STONE) {
        DrawRectangle(x, y, width, width, GRAY);
    } else if (cvalue.material == WATER) {
        Color color = ColorBrightness(BLUE, 0.5f - cvalue.state / 64.0f);
        DrawRectangle(x, y, width, width, color);
#ifdef DEBUG_CELL_INFO
        DrawText(TextFormat("%d", cvalue.state), x, y, 16, WHITE);
#endif
    } else if (cvalue.material == LAVA) {
        Color color = ColorBrightness(ORANGE, 0.5f - cvalue.state / 64.0f);
        DrawRectangle(x, y, width, width, color);
    }

    if (cvalue.type == VACUUM) {
        DrawRectangle(x, y, width, width, DARKGRAY);
        DrawRectangle(x, y, width / 2, width / 2, ColorBrightness(DARKGRAY, 0.1));
    }
}

void copyCellValue(const CellValue *source, CellValue *destination) {
    destination->type = source->type;
    destination->material = source->material;
    destination->state = source->state;
}

void setCellState(CellValue *cvalue, int state) {
    if (0 <= state) {
        cvalue->state = state;
    }
}

bool isEmpty(CellValue cvalue) { return cvalue.type == VACUUM; }

bool isFluid(CellValue cvalue) { return cvalue.type == FLUID; }

int difference(CellValue left, CellValue right) { return left.state - right.state; }
