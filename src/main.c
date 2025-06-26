/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit
https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "cell.h"
#include "raylib.h"
#include "raymath.h"

#define WIDTH 1280
#define HEIGHT 800

typedef struct GameData {
    Grid grid;
    int gridx;
    int gridy;
    int gridScale;

    Camera2D camera;
} GameData;

static GameData gameData;

void update() {
    if (IsKeyDown(KEY_W)) {
        gameData.camera.offset.y += 1;
    }
    if (IsKeyDown(KEY_S)) {
        gameData.camera.offset.y -= 1;
    }
    if (IsKeyDown(KEY_D)) {
        gameData.camera.offset.x -= 1;
    }
    if (IsKeyDown(KEY_A)) {
        gameData.camera.offset.x += 1;
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

void draw() {
    BeginDrawing();

    ClearBackground(BLACK);

    BeginMode2D(gameData.camera);

    drawGrid(gameData.gridx, gameData.gridy, gameData.gridScale, gameData.gridScale / 8, gameData.grid);

    EndMode2D();

    DrawText(TextFormat("%f", gameData.camera.zoom), 10, 10, 20, WHITE);
    
    Vector2 centerWorldPos = GetScreenToWorld2D((Vector2){WIDTH / 2.0, HEIGHT / 2.0}, gameData.camera);
    DrawText(TextFormat("(%f, %f)", centerWorldPos.x, centerWorldPos.y), 10, 30, 20, BLUE);
    DrawFPS(WIDTH - 80, HEIGHT - 30);

    EndDrawing();
}

void initGameData() {
    initGrid(&gameData.grid, 32, 32);
    gameData.gridScale = 24;
    gameData.gridx = -gridDrawWidth(gameData.gridScale, gameData.grid) / 2;
    gameData.gridy = -gridDrawHeight(gameData.gridScale, gameData.grid) / 2;

    gameData.camera = (Camera2D){.offset = (Vector2){WIDTH / 2.0, HEIGHT / 2.0}, .zoom = 1.0f};
}

void freeGameData() { freeGrid(&gameData.grid); }

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
