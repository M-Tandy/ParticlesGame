#include <stdlib.h>

#include "cell.h"
#include "common.h"
#include "debug.h"
#include "draw.h"
#include "quadtree.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "table.h"
#include "ui.h"

#define WIDTH 1600
#define HEIGHT 900
#define CELLPOWER 5
#define GRIDWIDTH 2048.0f / 2
#define UPDATE_RATE 60
#define FLUID_AMOUNT 64

typedef enum {
    TITLE,
    GRID,
    QUADTREE,
} Scene;

typedef enum { ADD, DELETE } Mode;

typedef struct GameData {
    Scene scene;

    Grid grid1;
    Grid grid2;
    int gridx;
    int gridy;
    int gridScale;

    QuadTree *quadtree;

    Mode mode;

    Camera2D camera;
    float timer;

    Button buttonStart;
    Button buttonQuadTree;

    bool paused;
} GameData;

static GameData gameData;

Table quadtrees;

const Vector2 origin = (Vector2){0.0f, 0.0f};

bool mouseDown(MouseButton *button) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        *button = MOUSE_BUTTON_LEFT;
        return true;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        *button = MOUSE_BUTTON_RIGHT;
        return true;
    }

    return false;
}

void toGrid() { gameData.scene = GRID; }

void toQuadTree() { gameData.scene = QUADTREE; }

void initGameData() {
    gameData.scene = TITLE;

    initGrid(&gameData.grid1, 32, 32, "Grid 1");
    initGrid(&gameData.grid2, 32, 32, "Grid 2");
    gameData.gridScale = 24;
    gameData.gridx = -gridDrawWidth(gameData.gridScale, gameData.grid1) / 2;
    gameData.gridy = -gridDrawHeight(gameData.gridScale, gameData.grid1) / 2;

    initQuadTable();
    gameData.quadtree = newEmptyQuadTree(CELLPOWER);

    gameData.mode = ADD;

    gameData.camera = (Camera2D){.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0}, .zoom = 1.0f};
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

    if (IsKeyPressed(KEY_O)) {
        Vector2 centerWorldPos = GetScreenToWorld2D((Vector2){WIDTH / 2.0, HEIGHT / 2.0}, gameData.camera);
        gameData.camera.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0};
        gameData.camera.target = centerWorldPos;
        if (gameData.camera.zoom > 1.0f) {
            gameData.camera.zoom -= 1.0f;
        } else {
            gameData.camera.zoom /= 2.0f;
        }
    }
    if (IsKeyPressed(KEY_P) && gameData.camera.zoom < 8.0f) {
        Vector2 centerWorldPos = GetScreenToWorld2D((Vector2){WIDTH / 2.0, HEIGHT / 2.0}, gameData.camera);
        gameData.camera.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0};
        gameData.camera.target = centerWorldPos;
        if (gameData.camera.zoom > 1.0f) {
            gameData.camera.zoom += 1.0f;
        } else {
            gameData.camera.zoom *= 2.0f;
        }
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

    if (gameData.timer > 1.0f/(float)UPDATE_RATE) {
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

    MouseButton button;
    if (mouseDown(&button)) {
        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
        if (IN_SQUARE(mousePos, origin, GRIDWIDTH)) {
            QuadTree *newTree = NULL;

            if (button == MOUSE_BUTTON_LEFT) {
                FluidValue newFluid = (FluidValue){FLUID_WATER, FLUID_AMOUNT};
                newTree = setPointInQuadTree(mousePos, origin, GRIDWIDTH, gameData.quadtree, FLUID_VALUE(newFluid));
            } else if (button == MOUSE_BUTTON_RIGHT) {
                newTree = setPointInQuadTree(mousePos, origin, GRIDWIDTH, gameData.quadtree, INT_VALUE(0));
            }

            if (newTree != NULL) {
                gameData.quadtree = newTree;
            }
        }
    }

    if (IsKeyPressed(KEY_L)) {
        printTreeTable();
    }

    if (IsKeyPressed(KEY_SPACE)) {
        gameData.paused = !gameData.paused;
    }

    if (IsKeyPressed(KEY_F) && gameData.paused) {
        gameData.quadtree = evolveQuadtree(gameData.quadtree);
    }

    if (!gameData.paused) {
        if (gameData.timer > 1.0f / (float)UPDATE_RATE) {
            gameData.quadtree = evolveQuadtree(gameData.quadtree);

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

    float gridCellSize = miniumumQuadSize(GRIDWIDTH, gameData.quadtree);
    int cells = maxQuads(gameData.quadtree);

    drawQuadTree(*gameData.quadtree, origin, GRIDWIDTH, gameData.camera);
    // drawQuadFromPosition(mousePos, gameData.quadtree, (Vector2){0.0f, 0.0f}, GRIDWIDTH);
    drawGridUnderlay(origin, cells, cells, gridCellSize);

    EndMode2D();
    DrawText(TextFormat("%d, %f", cells, gridCellSize), 100, 200, 32, WHITE);

    DrawText(TextFormat("%s", gameData.mode == ADD ? "ADD" : "DELETE"), 100, HEIGHT - 200, 32, WHITE);
    DrawText(TextFormat("%s", gameData.paused ? "Paused" : ""), WIDTH - 200, 200, 32, RED);
#ifdef DEBUG_QUADINFO
    DrawText(TextFormat("%p", gameData.quadtree), 200, HEIGHT - 100, 32, WHITE);
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

    SetTraceLogCallback(CustomLog);

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
