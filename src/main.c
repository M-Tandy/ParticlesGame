#include <stdlib.h>

#include "common.h"
#include "debug.h"
#include "draw.h"
#include "grid.h"
#include "logic.h"
#include "quadtree.h"
#include "table.h"
#include "ui.h"

static GameData gameData;

Table quadtrees;

// BUTTON STUFF
void toGrid() { gameData.scene = GRID; }

void toQuadTree() { gameData.scene = QUADTREE; }

void nextPlacing() {
    switch (gameData.placing.material) {

    case NONE:
        gameData.placing.type = GAS;
        gameData.placing.material = AIR;
        break;
    case AIR:
        gameData.placing.type = FLUID;
        gameData.placing.material = WATER;
        break;
    case WATER:
        gameData.placing.type = FLUID;
        gameData.placing.material = LAVA;
        break;
    case LAVA:
        gameData.placing.type = SOLID;
        gameData.placing.material = STONE;
        break;
    case STONE:
        gameData.placing.type = VACUUM;
        gameData.placing.material = NONE;
        break;
    }
}

void incrementPlacingState() { gameData.placing.state++; }

void decrementPlacingState() { gameData.placing.state--; }

void initGameData() {
    gameData.scene = TITLE;

    // GRID //
    initGrid(&gameData.grid1, GRID_WIDTH, GRID_WIDTH);
    initGrid(&gameData.grid2, GRID_WIDTH, GRID_WIDTH);
    gameData.gridWidthScale = GRID_RENDER_WIDTH / GRID_WIDTH;
    gameData.gridHeightScale = GRID_RENDER_HEIGHT / GRID_WIDTH;
    gameData.gridWidth = GRID_WIDTH;
    gameData.gridx = -gameData.gridWidthScale * GRID_WIDTH / 2;
    gameData.gridy = -gameData.gridHeightScale * GRID_WIDTH / 2;
    gameData.gridTexture = LoadRenderTexture(GRID_WIDTH, GRID_WIDTH);

    // QUADTABLE //
    initQuadTable();
    gameData.quadtree = newEmptyQuadTree(CELLPOWER);

    // CAMERA //
    gameData.camera = (Camera2D){.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0},
                                 .zoom = 1.0f};
    gameData.timer = 0.0f;

    // UI //
    gameData.buttonStart =
        newButton((Rectangle){WIDTH / 2 - 200 / 2 - 200, HEIGHT / 2, 200, 100},
                  true, "Grid", 32, toGrid);
    gameData.buttonQuadTree =
        newButton((Rectangle){WIDTH / 2 - 200 / 2 + 200, HEIGHT / 2, 200, 100},
                  true, "QuadTree", 32, toQuadTree);
    gameData.buttonNextMaterial = newButton(
        (Rectangle){WIDTH - 50, 200, 50, 50}, true, "->", 32, nextPlacing);
    gameData.buttonIncrementState =
        newButton((Rectangle){WIDTH - 50, 300, 50, 50}, true, "+", 32,
                  incrementPlacingState);
    gameData.buttonDecrementState =
        newButton((Rectangle){WIDTH - 50, 400, 50, 50}, true, "-", 32,
                  decrementPlacingState);

    // INTERACTION //
    gameData.placing = newCellValue(VACUUM, NONE, 0);
    gameData.paused = true;
}

void freeGameData() {
    freeGrid(&gameData.grid1);
    freeGrid(&gameData.grid2);
    UnloadRenderTexture(gameData.gridTexture);
}

int main() {
    // Tell the window to use vsync and work on high DPI displays
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

    SetTraceLogCallback(CustomLog);

    // Create the window and OpenGL context
    InitWindow(WIDTH, HEIGHT, "Cellular Alpha 0.0.1");

    // Initial game state
    initGameData();
    SetTargetFPS(60);

    while (!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE)) {
        update(&gameData);
        draw(&gameData);
    }

    freeGameData();

    // destroy the window and cleanup the OpenGL context
    CloseWindow();
    return 0;
}
