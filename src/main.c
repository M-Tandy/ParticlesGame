#include <stdlib.h>

#include "common.h"
#include "debug.h"
#include "draw.h"
#include "grid.h"
#include "quadtree.h"
#include "raylib.h"
#include "rlgl.h"
#include "table.h"
#include "ui.h"

#define WIDTH 1728
#define HEIGHT 1024
#define CELLPOWER 5
#define GRIDWIDTH 2048.0f / 2
#define UPDATE_RATE 60
#define FLUID_AMOUNT 64

#define CAMERA_SPEED 8

#define GRID_WIDTH 1024
#define GRID_RENDER_WIDTH 1024
#define GRID_RENDER_HEIGHT 1024

typedef enum {
    TITLE,
    GRID,
    QUADTREE,
} Scene;

typedef struct GameData {
    Scene scene;

    CellValue placing;

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

    bool paused;
} GameData;

static GameData gameData;

Table quadtrees;

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

    gameData.placing = newCellValue(VACUUM, NONE, 0);

    // Grid
    initGrid(&gameData.grid1, GRID_WIDTH, GRID_WIDTH);
    initGrid(&gameData.grid2, GRID_WIDTH, GRID_WIDTH);
    gameData.gridWidthScale = GRID_RENDER_WIDTH / GRID_WIDTH;
    gameData.gridHeightScale = GRID_RENDER_HEIGHT / GRID_WIDTH;
    gameData.gridWidth = GRID_WIDTH;
    gameData.gridx = -gameData.gridWidthScale * GRID_WIDTH / 2;
    gameData.gridy = -gameData.gridHeightScale * GRID_WIDTH / 2;
    gameData.gridTexture = LoadRenderTexture(GRID_WIDTH, GRID_WIDTH);

    initQuadTable();
    gameData.quadtree = newEmptyQuadTree(CELLPOWER);

    gameData.camera = (Camera2D){.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0}, .zoom = 1.0f};
    gameData.timer = 0.0f;

    gameData.buttonStart =
        newButton((Rectangle){WIDTH / 2 - 200 / 2 - 200, HEIGHT / 2, 200, 100}, true, "Grid", 32, toGrid);
    gameData.buttonQuadTree =
        newButton((Rectangle){WIDTH / 2 - 200 / 2 + 200, HEIGHT / 2, 200, 100}, true, "QuadTree", 32, toQuadTree);
    gameData.buttonNextMaterial = newButton((Rectangle){WIDTH - 50, 200, 50, 50}, true, "->", 32, nextPlacing);
    gameData.buttonIncrementState =
        newButton((Rectangle){WIDTH - 50, 300, 50, 50}, true, "+", 32, incrementPlacingState);
    gameData.buttonDecrementState =
        newButton((Rectangle){WIDTH - 50, 400, 50, 50}, true, "-", 32, decrementPlacingState);

    gameData.paused = true;
}

void freeGameData() {
    freeGrid(&gameData.grid1);
    freeGrid(&gameData.grid2);
    UnloadRenderTexture(gameData.gridTexture);
}

void updateSceneTitle() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        tryButtonPress(gameData.buttonStart);
        tryButtonPress(gameData.buttonQuadTree);
    }
}

void cameraUpdate() {
    if (IsKeyDown(KEY_W)) {
        gameData.camera.offset.y += CAMERA_SPEED * gameData.camera.zoom;
    }
    if (IsKeyDown(KEY_S)) {
        gameData.camera.offset.y -= CAMERA_SPEED * gameData.camera.zoom;
    }
    if (IsKeyDown(KEY_D)) {
        gameData.camera.offset.x -= CAMERA_SPEED * gameData.camera.zoom;
    }
    if (IsKeyDown(KEY_A)) {
        gameData.camera.offset.x += CAMERA_SPEED * gameData.camera.zoom;
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

static bool buttonsSceneGrid() {
    return tryButtonPress(gameData.buttonIncrementState) || tryButtonPress(gameData.buttonDecrementState) ||
           tryButtonPress(gameData.buttonNextMaterial);
}

void updateSceneGrid() {
    cameraUpdate();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        buttonsSceneGrid();
    }

    MouseButton button;
    if (mouseDown(&button)) {
        Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
        if (button == MOUSE_BUTTON_LEFT) {
            CellValue *result = NULL;
            if (getCellAt(&gameData.grid1, gameData.gridx, gameData.gridy, worldPos.x, worldPos.y,
                          gameData.gridWidthScale, gameData.gridHeightScale, &result)) {
                *result = gameData.placing;
                result->settled = false;
            }
        } else if (button == MOUSE_BUTTON_RIGHT) {
            CellValue *result = NULL;
            if (getCellAt(&gameData.grid1, gameData.gridx, gameData.gridy, worldPos.x, worldPos.y,
                          gameData.gridWidthScale, gameData.gridHeightScale, &result)) {
                result->material = NONE;
                result->type = VACUUM;
                result->state = 0;
            }
        }
    }

    if (IsKeyDown(KEY_L)) {
        Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
        CellValue *result = NULL;
        if (getCellAt(&gameData.grid1, gameData.gridx, gameData.gridy, worldPos.x, worldPos.y, gameData.gridWidthScale,
                      gameData.gridHeightScale, &result)) {
            result->material = LAVA;
            result->type = FLUID;
            result->state = 32;
            result->settled = false;
        }
    }
    if (IsKeyDown(KEY_T)) {
        Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
        CellValue *result = NULL;
        if (getCellAt(&gameData.grid1, gameData.gridx, gameData.gridy, worldPos.x, worldPos.y, gameData.gridWidthScale,
                      gameData.gridHeightScale, &result)) {
            result->material = STONE;
            result->type = SOLID;
            result->state = 32;
            result->settled = false;
        }
    }

    if (IsKeyPressed(KEY_SPACE)) {
        gameData.paused = !gameData.paused;
    }

    if (IsKeyPressed(KEY_F) && gameData.paused) {
        evolveGrid(&gameData.grid1, &gameData.grid2);

        // Swapping grids
        Grid temp = gameData.grid1;
        gameData.grid1 = gameData.grid2;
        gameData.grid2 = temp;

        gameData.timer = 0.0f;
    }

    if (!gameData.paused && gameData.timer > 1.0f / (float)UPDATE_RATE) {
        evolveGrid(&gameData.grid1, &gameData.grid2);

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
        if (IN_SQUARE(mousePos, ORIGIN, GRIDWIDTH)) {
            QuadTree *newTree = NULL;

            if (button == MOUSE_BUTTON_LEFT) {
                FluidValue newFluid = (FluidValue){FLUID_WATER, FLUID_AMOUNT};
                newTree = setPointInQuadTree(mousePos, ORIGIN, GRIDWIDTH, gameData.quadtree, FLUID_VALUE(newFluid));
            } else if (button == MOUSE_BUTTON_RIGHT) {
                newTree = setPointInQuadTree(mousePos, ORIGIN, GRIDWIDTH, gameData.quadtree, INT_VALUE(0));
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

static void drawButtonsSceneGrid() {
    drawButton(gameData.buttonNextMaterial);
    drawButton(gameData.buttonIncrementState);
    drawButton(gameData.buttonDecrementState);
}

void drawSceneGrid() {
    ClearBackground(BLACK);

    BeginMode2D(gameData.camera);

    // drawGrid(&gameData.grid1, gameData.gridx, gameData.gridy, gameData.gridScale, gameData.gridScale, 0);

    DrawTexturePro(
        gameData.gridTexture.texture,
        (Rectangle){0, 0, (float)gameData.gridTexture.texture.width, -(float)gameData.gridTexture.texture.height},
        (Rectangle){gameData.gridx, gameData.gridy, gameData.gridWidthScale * gameData.gridWidth,
                    gameData.gridHeightScale * gameData.gridWidth},
        (Vector2){0, 0}, 0.0f, WHITE);

    EndMode2D();

    drawCellValue(gameData.placing, WIDTH - 100, 50, 50, 50);
    DrawText(TextFormat("%d", gameData.placing.state), WIDTH - 90, 60, 32, WHITE);
    drawButtonsSceneGrid();

    DrawText(TextFormat("%f", gameData.camera.zoom), 10, 10, 20, WHITE);
    DrawText(TextFormat("Time: %f", gameData.timer), 10, 30, 20, BLUE);
    DrawText(TextFormat("%s", gameData.paused ? "Paused" : ""), WIDTH - 200, 200, 32, RED);
}

void drawSceneQuadTree() {
    ClearBackground(BLACK);

    BeginMode2D(gameData.camera);

    float gridCellSize = miniumumQuadSize(GRIDWIDTH, gameData.quadtree);
    int cells = maxQuads(gameData.quadtree);

    drawQuadTree(*gameData.quadtree, ORIGIN, GRIDWIDTH, gameData.camera);
    // drawQuadFromPosition(mousePos, gameData.quadtree, (Vector2){0.0f, 0.0f}, GRIDWIDTH);
    drawGridUnderlay(ORIGIN, cells, cells, gridCellSize);

    EndMode2D();
    DrawText(TextFormat("%d, %f", cells, gridCellSize), 100, 200, 32, WHITE);

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
        BeginTextureMode(gameData.gridTexture);
        // clang-format off
            // ClearBackground(BLANK);
            drawGridPixels(&gameData.grid1, 0, 0);
        // clang-format on
        EndTextureMode();
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
