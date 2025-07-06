/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit
https://creativecommons.org/publicdomain/zero/1.0/

*/

#include <stdlib.h>

#include "cell.h"
#include "common.h"
#include "draw.h"
#include "quadtree.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "ui.h"

#define WIDTH 1600
#define HEIGHT 900

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
    int gridScale;

    QuadTree *quadtree;
    QuadTree *quadtreeAlt;

    Camera2D camera;
    float timer;

    Button buttonStart;
    Button buttonQuadTree;

    bool paused;
} GameData;

static GameData gameData;

void toGrid() { gameData.scene = GRID; }

void toQuadTree() { gameData.scene = QUADTREE; }

void initGameData() {
    gameData.scene = TITLE;

    initGrid(&gameData.grid1, 32, 32, "Grid 1");
    initGrid(&gameData.grid2, 32, 32, "Grid 2");
    gameData.gridScale = 24;
    gameData.gridx = -gridDrawWidth(gameData.gridScale, gameData.grid1) / 2;
    gameData.gridy = -gridDrawHeight(gameData.gridScale, gameData.grid1) / 2;

    gameData.quadtree = malloc(sizeof(QuadTree));
    gameData.quadtreeAlt = malloc(sizeof(QuadTree));

    initQuadTree(gameData.quadtree, 0);
    initQuadTree(gameData.quadtreeAlt, 0);

    // REQUIRED. Things break if the quadtree is not subdivided at all
    fullySubdivide(gameData.quadtree);
    fullySubdivide(gameData.quadtreeAlt);

    gameData.camera = (Camera2D){.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0}, .zoom = 1.0f};
    // For drawing both quads
    // gameData.camera = (Camera2D){.offset = (Vector2){WIDTH / 2.0 - 400, HEIGHT / 2.0}, .zoom = 1.0f};
    gameData.timer = 0.0f;

    gameData.buttonStart =
        newButton((Rectangle){WIDTH / 2 - 200 / 2 - 200, HEIGHT / 2, 200, 100}, true, "Grid", 32, toGrid);
    gameData.buttonQuadTree =
        newButton((Rectangle){WIDTH / 2 - 200 / 2 + 200, HEIGHT / 2, 200, 100}, true, "QuadTree", 32, toQuadTree);

    gameData.paused = true;
}

void freeGameData() {
    freeGrid(&gameData.grid1);
    freeGrid(&gameData.grid2);

    freeQuadTree(gameData.quadtree);
    freeQuadTree(gameData.quadtreeAlt);

    free(gameData.quadtree);
    free(gameData.quadtreeAlt);
}

void updateSceneTitle() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        tryButtonPress(gameData.buttonStart);
        tryButtonPress(gameData.buttonQuadTree);
    }
}

void cameraUpdate() {
    if (IsKeyDown(KEY_W)) {
        gameData.camera.offset.y += 2 * gameData.camera.zoom;
    }
    if (IsKeyDown(KEY_S)) {
        gameData.camera.offset.y -= 2 * gameData.camera.zoom;
    }
    if (IsKeyDown(KEY_D)) {
        gameData.camera.offset.x -= 2 * gameData.camera.zoom;
    }
    if (IsKeyDown(KEY_A)) {
        gameData.camera.offset.x += 2 * gameData.camera.zoom;
    }

    if (IsKeyPressed(KEY_O) && gameData.camera.zoom > 1.0f) {
        Vector2 centerWorldPos = GetScreenToWorld2D((Vector2){WIDTH / 2.0, HEIGHT / 2.0}, gameData.camera);
        gameData.camera.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0};
        gameData.camera.target = centerWorldPos;
        gameData.camera.zoom -= 1.0f;
    }
    if (IsKeyPressed(KEY_P) && gameData.camera.zoom < 4.0f) {
        Vector2 centerWorldPos = GetScreenToWorld2D((Vector2){WIDTH / 2.0, HEIGHT / 2.0}, gameData.camera);
        gameData.camera.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0};
        gameData.camera.target = centerWorldPos;
        gameData.camera.zoom += 1.0f;
    }
}

void updateSceneGrid() {
    Vector2 mousePos = GetMousePosition();
    Vector2 worldPos = GetScreenToWorld2D(mousePos, gameData.camera);

    cameraUpdate();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        BoundingBox gridbbox = gridBoundingBox(gameData.gridx, gameData.gridy, gameData.gridScale, gameData.grid1);
        if (IN_BBOX(worldPos, gridbbox)) {
            Cell *gridCell = positionToGridCell(worldPos.x, worldPos.y, gameData.gridx, gameData.gridy,
                                                gameData.gridScale, &gameData.grid1);
            gridCell->state = !gridCell->state;
        }
    }

    if (gameData.timer > 0.5f) {
        updateGrids(&gameData.grid1, &gameData.grid2);

        // Swapping grids
        Grid temp = gameData.grid1;
        gameData.grid1 = gameData.grid2;
        gameData.grid2 = temp;

        gameData.timer = 0.0f;
    }
}

void updateSceneQuadTree() {
    cameraUpdate();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
        QuadrantValue *lowestTree = quadFromPosition(mousePos, gameData.quadtree, (Vector2){0.0f, 0.0f}, 512.0f);
        QuadrantValue *lowestTreeAlt = quadFromPosition(mousePos, gameData.quadtreeAlt, (Vector2){0.0f, 0.0f}, 512.0f);
        if (IS_QUADTREE(*lowestTree) && !isSubdivided(*AS_QUADTREE(*lowestTree))) {
            subdivide(AS_QUADTREE(*lowestTree));
            subdivide(AS_QUADTREE(*lowestTreeAlt));
        } else if (IS_INT(*lowestTree)) {
            AS_INT(*lowestTree) = ++AS_INT(*lowestTree) % 2;
            // AS_INT(*lowestTreeAlt) = ++AS_INT(*lowestTreeAlt) % 2;
        }
    }

    if (IsKeyPressed(KEY_SPACE)) {
        gameData.paused = !gameData.paused;
    }

    if (!gameData.paused) {
        if (gameData.timer > 0.1f) {
            evolveQuadtree(gameData.quadtree, gameData.quadtreeAlt);

            // Swapping the trees
            QuadTree *temp = gameData.quadtree;
            gameData.quadtree = gameData.quadtreeAlt;
            gameData.quadtreeAlt = temp;

            gameData.timer = 0.0f;
        }
    }
}

void update() {
    float dt = GetFrameTime();
    gameData.timer += dt;

    switch (gameData.scene) {

    case TITLE:
        updateSceneTitle();
        break;
    case GRID:
        updateSceneGrid();
        break;
    case QUADTREE:
        updateSceneQuadTree();
        break;
    }
}

void drawSceneTitle() {
    int textSize = MeasureText("Cellular Alpha", 64);
    DrawText("Cellular Alpha", WIDTH / 2 - textSize / 2, HEIGHT / 2 - 100, 64, WHITE);

    drawButton(gameData.buttonStart);
    drawButton(gameData.buttonQuadTree);
}

void drawSceneGrid() {
    ClearBackground(BLACK);

    BeginMode2D(gameData.camera);

    drawGrid(gameData.gridx, gameData.gridy, gameData.gridScale, gameData.gridScale / 8, gameData.grid1);

    BoundingBox gridbbox = gridBoundingBox(gameData.gridx, gameData.gridy, gameData.gridScale, gameData.grid1);
    DrawBoundingBox(gridbbox, RED);

    EndMode2D();

    DrawText(TextFormat("%f", gameData.camera.zoom), 10, 10, 20, WHITE);
    DrawText(TextFormat("Time: %f", gameData.timer), 10, 30, 20, BLUE);
    DrawText(TextFormat("Time: %s", gameData.grid1.label), 10, 50, 20, BLUE);
}

void drawSceneQuadTree() {
    ClearBackground(BLACK);

    BeginMode2D(gameData.camera);

    float gridCellSize = miniumumQuadSize(512.0f);
    int cells = maxQuads();

    drawQuadTree(*gameData.quadtree, (Vector2){0.0f, 0.0f}, 512.0f, gameData.camera);
    // drawQuadTree(*gameData.quadtreeAlt, (Vector2){800.0f, 0.0f}, 512.0f, gameData.camera);
    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
    drawQuadFromPosition(mousePos, gameData.quadtree, (Vector2){0.0f, 0.0f}, 512.0f);
    drawGridUnderlay((Vector2){0.0f, 0.0f}, cells, cells, gridCellSize);

    EndMode2D();
    DrawText(TextFormat("%d, %f", cells, gridCellSize), 100, 200, 32, WHITE);

#ifdef DEBUG_QUADINFO
    DrawText(TextFormat("%p", gameData.quadtree), 200, HEIGHT - 100, 32, WHITE);
    DrawText(TextFormat("%p", gameData.quadtreeAlt), 800, HEIGHT - 100, 32, WHITE);
#endif
}

void draw() {
    BeginDrawing();

    ClearBackground(BLACK);

    switch (gameData.scene) {

    case TITLE:
        drawSceneTitle();
        break;
    case GRID:
        drawSceneGrid();
        break;
    case QUADTREE:
        drawSceneQuadTree();
        break;
    }

    DrawFPS(WIDTH - 80, HEIGHT - 30);

    EndDrawing();
}

int main() {
    // Tell the window to use vsync and work on high DPI displays
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

    // Create the window and OpenGL context
    InitWindow(WIDTH, HEIGHT, "Cellular Alpha 0.0.1");

    // Initial game state
    initGameData();
    SetTargetFPS(60);

    while (!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE)) {
        update();
        draw();
    }

    freeGameData();

    // destroy the window and cleanup the OpenGL context
    CloseWindow();
    return 0;
}
