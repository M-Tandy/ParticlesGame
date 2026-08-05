#include "logic.h"
#include "common.h"

static void updateSceneTitle(GameData *data) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        tryButtonPress(data->buttonStart);
        tryButtonPress(data->buttonQuadTree);
    }
}

static void cameraUpdate(GameData *gameData) {
    if (IsKeyDown(KEY_W)) {
        gameData->camera.offset.y += CAMERA_SPEED * gameData->camera.zoom;
    }
    if (IsKeyDown(KEY_S)) {
        gameData->camera.offset.y -= CAMERA_SPEED * gameData->camera.zoom;
    }
    if (IsKeyDown(KEY_D)) {
        gameData->camera.offset.x -= CAMERA_SPEED * gameData->camera.zoom;
    }
    if (IsKeyDown(KEY_A)) {
        gameData->camera.offset.x += CAMERA_SPEED * gameData->camera.zoom;
    }

    if (IsKeyPressed(KEY_O)) {
        Vector2 centerWorldPos = GetScreenToWorld2D((Vector2){WIDTH / 2.0, HEIGHT / 2.0}, gameData->camera);
        gameData->camera.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0};
        gameData->camera.target = centerWorldPos;
        if (gameData->camera.zoom > 1.0f) {
            gameData->camera.zoom -= 1.0f;
        } else {
            gameData->camera.zoom /= 2.0f;
        }
    }
    if (IsKeyPressed(KEY_P) && gameData->camera.zoom < 8.0f) {
        Vector2 centerWorldPos = GetScreenToWorld2D((Vector2){WIDTH / 2.0, HEIGHT / 2.0}, gameData->camera);
        gameData->camera.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0};
        gameData->camera.target = centerWorldPos;
        if (gameData->camera.zoom > 1.0f) {
            gameData->camera.zoom += 1.0f;
        } else {
            gameData->camera.zoom *= 2.0f;
        }
    }
}

static bool buttonsSceneGrid(GameData *gameData) {
    return tryButtonPress(gameData->buttonIncrementState) || tryButtonPress(gameData->buttonDecrementState) ||
           tryButtonPress(gameData->buttonNextMaterial);
}

static void gridUpdateAndSwap(GameData *gameData) {
    evolveGrid(&gameData->grid1, &gameData->grid2);

    // Swapping grids
    Grid temp = gameData->grid1;
    gameData->grid1 = gameData->grid2;
    gameData->grid2 = temp;
}

static void updateSceneGrid(GameData *gameData) {
    cameraUpdate(gameData);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        buttonsSceneGrid(gameData);
    }

    MouseButton button;
    if (mouseDown(&button)) {
        Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData->camera);
        if (button == MOUSE_BUTTON_LEFT) {
            CellValue *result = NULL;
            if (getCellAt(&gameData->grid1, gameData->gridx, gameData->gridy, worldPos.x, worldPos.y,
                          gameData->gridWidthScale, gameData->gridHeightScale, &result)) {
                *result = gameData->placing;
                result->settled = false;
            }
        } else if (button == MOUSE_BUTTON_RIGHT) {
            CellValue *result = NULL;
            if (getCellAt(&gameData->grid1, gameData->gridx, gameData->gridy, worldPos.x, worldPos.y,
                          gameData->gridWidthScale, gameData->gridHeightScale, &result)) {
                result->material = NONE;
                result->type = VACUUM;
                result->state = 0;
            }
        }
    }

    if (IsKeyPressed(KEY_SPACE)) {
        gameData->paused = !gameData->paused;
    }

    if ((!gameData->paused && gameData->timer > 1.0f / (float)UPDATE_RATE) || (gameData->paused && IsKeyPressed(KEY_F))) {
        gridUpdateAndSwap(gameData);
        gameData->timer = 0.0f;
    }
}

static void updateSceneQuadTree(GameData *gameData) {
    cameraUpdate(gameData);

    MouseButton button;
    if (mouseDown(&button)) {
        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), gameData->camera);
        if (IN_SQUARE(mousePos, ORIGIN, GRIDWIDTH)) {
            QuadTree *newTree = NULL;

            if (button == MOUSE_BUTTON_LEFT) {
                FluidValue newFluid = (FluidValue){FLUID_WATER, FLUID_AMOUNT};
                newTree = setPointInQuadTree(mousePos, ORIGIN, GRIDWIDTH, gameData->quadtree, FLUID_VALUE(newFluid));
            } else if (button == MOUSE_BUTTON_RIGHT) {
                newTree = setPointInQuadTree(mousePos, ORIGIN, GRIDWIDTH, gameData->quadtree, INT_VALUE(0));
            }

            if (newTree != NULL) {
                gameData->quadtree = newTree;
            }
        }
    }

    if (IsKeyPressed(KEY_L)) {
        printTreeTable();
    }

    if (IsKeyPressed(KEY_SPACE)) {
        gameData->paused = !gameData->paused;
    }

    if (IsKeyPressed(KEY_F) && gameData->paused) {
        gameData->quadtree = evolveQuadtree(gameData->quadtree);
    }

    if (!gameData->paused) {
        if (gameData->timer > 1.0f / (float)UPDATE_RATE) {
            gameData->quadtree = evolveQuadtree(gameData->quadtree);

            gameData->timer = 0.0f;
        }
    }
}

void update(GameData *data) {
    float dt = GetFrameTime();
    data->timer += dt;

    switch (data->scene) {

    case TITLE:
        updateSceneTitle(data);
        break;
    case GRID:
        updateSceneGrid(data);
        break;
    case QUADTREE:
        updateSceneQuadTree(data);
        break;
    }
}
