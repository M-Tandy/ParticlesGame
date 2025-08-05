#ifndef ptest_value_h
#define ptest_value_h

#include "common.h"

// For calculating cell spread
typedef struct OccupationNumber {
    int nw;
    int n;
    int ne;
    int w;
    int c;
    int e;
    int sw;
    int s;
    int se;
} OccupationNumber;

typedef enum {
    VACUUM,
    SOLID,
    FLUID,
    GAS,
} CType;

typedef enum {
    NONE,
    AIR,
    WATER,
    LAVA,
    STONE,
} CMaterial;

typedef struct CellValue {
    CType type;
    CMaterial material;
    int state;
    OccupationNumber occ;
    bool settled;
} CellValue;

void initOccupationNumber(OccupationNumber *occ);
void initCellValue(CellValue *cvalue, CType type, CMaterial material, int state);
CellValue newCellValue(CType type, CMaterial material, int state);
Color cellColor(CellValue cvalue);
void drawCellValue(CellValue cvalue, int x, int y, int width, int height);
void copyCellValue(const CellValue *source, CellValue *destination);
void setCellState(CellValue *cvalue, int state);

bool isEmpty(CellValue cvalue);
bool isFluid(CellValue cvalue);
int difference(CellValue left, CellValue right);

#endif // ptest_value_h
