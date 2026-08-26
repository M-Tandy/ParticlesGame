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

typedef enum {
    MAT_STONE,
} SolidMaterial;

typedef struct CellOld {
    CType type;
    CMaterial material;
    int state;
    double doubleState;
    OccupationNumber occ;
    bool settled;
} CellOld;

typedef struct Cell {
    CType type;
    CMaterial material;
    double state;
    bool settled;

    OccupationNumber occ;
} Cell;

// clang-format off
#define IS_EMPTY(value) ((value).type == NONE)
#define IS_SOLID(value) ((value).type == SOLID)
#define IS_FLUID(value) ((value).type == FLUID)
#define IS_GAS(value)   ((value).type == GAS)
// clang-format on

void InitOccupationNumber(OccupationNumber *occ);
void InitCell(Cell *cvalue, CType type, CMaterial material, int state);
Cell NewCell(CType type, CMaterial material, int state);
Color CellColor(Cell cvalue);
void DrawCell(Cell cvalue, int x, int y, int width, int height);
void CopyCell(const Cell *source, Cell *destination);
void SetCellState(Cell *cvalue, int state);

bool IsEmpty(Cell cvalue);
bool IsFluid(Cell cvalue);
int Difference(Cell left, Cell right);

#endif // ptest_value_h
