#ifndef ptest_data_h
#define ptest_data_h

#include "value.h"
#include "grid.h"
#include "quadtree.h"
#include "ui.h"

typedef enum {
    TITLE,
    GRID,
    QUADTREE,
} Scene;

typedef struct GameData {
    Scene scene;

    Grid grid1;
    Grid grid2;
    int gridx;
    int gridy;
    int gridWidthScale;
    int gridHeightScale;
    int gridWidth;
    RenderTexture2D gridTexture;

    QuadTree *quadtree;

    Camera2D camera;
    float timer;

    Button buttonStart;
    Button buttonQuadTree;
    Button buttonNextMaterial;
    Button buttonIncrementState;
    Button buttonDecrementState;

    CellValue placing;
    bool paused;
} GameData;

#endif // ptest_data_h

