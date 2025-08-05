#include <stdlib.h>
#include <time.h>

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
    int gridWidthScale;
    int gridHeightScale;
    int gridWidth;
    RenderTexture2D gridTexture;

    QuadTree *quadtree;

    Mode mode;

    Camera2D camera;
    float timer;

    Button buttonStart;
    Button buttonQuadTree;

    bool paused;
} GameData;

static GameData gameData;
static bool logFlag;

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

    int width = 1024;
    int renderWidth = 1024;
    int renderHeight = 1024;
    initGrid(&gameData.grid1, width, width);
    initGrid(&gameData.grid2, width, width);
    gameData.gridWidthScale = renderWidth / width;
    gameData.gridHeightScale = renderHeight / width;
    gameData.gridWidth = width;

    gameData.gridx = -gameData.gridWidthScale * width / 2;
    gameData.gridy = -gameData.gridHeightScale * width / 2;
    gameData.gridTexture = LoadRenderTexture(width, width);

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

    logFlag = false;
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

void updateSceneGrid() {
    cameraUpdate();

    MouseButton button;
    if (mouseDown(&button)) {
        Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
        if (button == MOUSE_BUTTON_LEFT) {
            CellValue *result = NULL;
            if (getCellAt(&gameData.grid1, gameData.gridx, gameData.gridy, worldPos.x, worldPos.y, gameData.gridWidthScale,
                          gameData.gridHeightScale, &result)) {
                result->material = WATER;
                result->type = FLUID;
                result->state = 32;
                result->settled = false;
            }
        } else if (button == MOUSE_BUTTON_RIGHT) {
            CellValue *result = NULL;
            if (getCellAt(&gameData.grid1, gameData.gridx, gameData.gridy, worldPos.x, worldPos.y, gameData.gridWidthScale,
                          gameData.gridHeightScale, &result)) {
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

    if (IsKeyPressed(KEY_I)) {
        logFlag = !logFlag;
    }

    if (!gameData.paused && gameData.timer > 1.0f / (float)UPDATE_RATE) {

        clock_t begin = 0;
        if (logFlag) {
            begin = clock();
        }

        evolveGrid(&gameData.grid1, &gameData.grid2);

        // Swapping grids
        Grid temp = gameData.grid1;
        gameData.grid1 = gameData.grid2;
        gameData.grid2 = temp;

        gameData.timer = 0.0f;

        if (logFlag) {
            clock_t end = clock();
            double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
            LogMessage(LOG_INFO, "Time to update: %f secs", time_spent);
        }
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

    // drawGrid(&gameData.grid1, gameData.gridx, gameData.gridy, gameData.gridScale, gameData.gridScale, 0);

    DrawTexturePro(
        gameData.gridTexture.texture,
        (Rectangle){0, 0, (float)gameData.gridTexture.texture.width, -(float)gameData.gridTexture.texture.height},
        (Rectangle){gameData.gridx, gameData.gridy, gameData.gridWidthScale * gameData.gridWidth,
                    gameData.gridHeightScale * gameData.gridWidth},
        (Vector2){0, 0}, 0.0f, WHITE);

    EndMode2D();

    DrawText(TextFormat("%f", gameData.camera.zoom), 10, 10, 20, WHITE);
    DrawText(TextFormat("Time: %f", gameData.timer), 10, 30, 20, BLUE);
    DrawText(TextFormat("%s", gameData.paused ? "Paused" : ""), WIDTH - 200, 200, 32, RED);
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
