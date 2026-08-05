#include "draw.h"
#include "data.h"

void drawCenteredSquareLines(Vector2 pos, float width, Color color) {
    DrawRectangleLines(pos.x - width / 2, pos.y - width / 2, width, width, color);
}

void drawCenteredSquare(Vector2 pos, float width, Color color) {
    DrawRectangle(pos.x - width * 0.5, pos.y - width * 0.5, width, width, color);
}

// Draws a centred grid at `centre`.
void drawGridUnderlay(Vector2 centre, int rows, int cols, float spacing) {
    float width = cols * spacing;
    float height = rows * spacing;
    for (int i = 0; i < cols; i++) {
        for (int j = 0; j < rows; j++) {
            DrawRectangleLines(centre.x - width/2.0f + i*spacing, centre.y - height/2.0f + j * spacing, spacing, spacing, DARKGRAY);
        }
    }
}


static void drawSceneTitle(GameData *data) {
    int textSize = MeasureText("Cellular Alpha", 64);
    DrawText("Cellular Alpha", WIDTH / 2 - textSize / 2, HEIGHT / 2 - 100, 64, WHITE);

    drawButton(data->buttonStart);
    drawButton(data->buttonQuadTree);
}

static void drawButtonsSceneGrid(GameData *data) {
    drawButton(data->buttonNextMaterial);
    drawButton(data->buttonIncrementState);
    drawButton(data->buttonDecrementState);
}

static void drawSceneGrid(GameData *data) {
    ClearBackground(BLACK);

    BeginMode2D(data->camera);

    // drawGrid(&gameData->grid1, gameData->gridx, gameData->gridy, gameData->gridScale, gameData->gridScale, 0);

    DrawTexturePro(
        data->gridTexture.texture,
        (Rectangle){0, 0, (float)data->gridTexture.texture.width, -(float)data->gridTexture.texture.height},
        (Rectangle){data->gridx, data->gridy, data->gridWidthScale * data->gridWidth,
                    data->gridHeightScale * data->gridWidth},
        (Vector2){0, 0}, 0.0f, WHITE);

    EndMode2D();

    drawCellValue(data->placing, WIDTH - 100, 50, 50, 50);
    DrawText(TextFormat("%d", data->placing.state), WIDTH - 90, 60, 32, WHITE);
    drawButtonsSceneGrid(data);

    DrawText(TextFormat("%f", data->camera.zoom), 10, 10, 20, WHITE);
    DrawText(TextFormat("Time: %f", data->timer), 10, 30, 20, BLUE);
    DrawText(TextFormat("%s", data->paused ? "Paused" : ""), WIDTH - 200, 200, 32, RED);
}

static void drawSceneQuadTree(GameData *data) {
    ClearBackground(BLACK);

    BeginMode2D(data->camera);

    float gridCellSize = miniumumQuadSize(GRIDWIDTH, data->quadtree);
    int cells = maxQuads(data->quadtree);

    drawQuadTree(*data->quadtree, ORIGIN, GRIDWIDTH, data->camera);
    // drawQuadFromPosition(mousePos, gameData->quadtree, (Vector2){0.0f, 0.0f}, GRIDWIDTH);
    drawGridUnderlay(ORIGIN, cells, cells, gridCellSize);

    EndMode2D();
    DrawText(TextFormat("%d, %f", cells, gridCellSize), 100, 200, 32, WHITE);

    DrawText(TextFormat("%s", data->paused ? "Paused" : ""), WIDTH - 200, 200, 32, RED);
#ifdef DEBUG_QUADINFO
    DrawText(TextFormat("%p", gameData->quadtree), 200, HEIGHT - 100, 32, WHITE);
#endif
}

void draw(GameData *data) {
    BeginDrawing();

    ClearBackground(BLACK);

    switch (data->scene) {

    case TITLE:
        drawSceneTitle(data);
        break;
    case GRID:
        BeginTextureMode(data->gridTexture);
        // clang-format off
            // ClearBackground(BLANK);
            drawGridPixels(&data->grid1, 0, 0);
        // clang-format on
        EndTextureMode();
        drawSceneGrid(data);
        break;
    case QUADTREE:
        drawSceneQuadTree(data);
        break;
    }

    DrawFPS(WIDTH - 80, HEIGHT - 30);

    EndDrawing();
}

