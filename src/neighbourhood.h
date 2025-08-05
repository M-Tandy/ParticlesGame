#ifndef ptest_neighbourhood_h
#define ptest_neighbourhood_h

#include "value.h"

typedef struct CellNeighbourhood {
    CellValue nw;
    CellValue n;
    CellValue ne;
    CellValue w;
    CellValue c;
    CellValue e;
    CellValue sw;
    CellValue s;
    CellValue se;
} CellNeighbourhood;

CellNeighbourhood newCellNeighbourhood(CellValue nw, CellValue n, CellValue ne, CellValue w, CellValue c, CellValue e, CellValue sw, CellValue s, CellValue se);
OccupationNumber collide(CellNeighbourhood n);
int surroundingSum(CellNeighbourhood n);
CMaterial determineMaterial(CellNeighbourhood n);
CellValue react(CellNeighbourhood n);

#endif // ptest_neighbourhood_h
