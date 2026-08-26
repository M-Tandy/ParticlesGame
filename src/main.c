/**
 ********************************************************************************
 * @file     main.c
 * @author   Matthew Tandy
 * @created  Unknown
 * @brief    Program startpoint. Contains startup and cleanup.
 ********************************************************************************
 */

// INCLUDES
#include <raylib.h>
#include <stdlib.h>

#include "common.h"
#include "debug.h"
#include "title.h"

// EXTERN VARIABLES

// PRIVATE MACROS AND DEFINES

// PRIVATE TYPEDEFS

// STATIC VARIABLES
static GameData gameData;

// GLOBAL VARIABLES

// STATIC FUNCTION PROTOTYPES

// STATIC FUNCTIONS
static void initGameData() {
    // CAMERA //
    // gameData.camera = (Camera2D){.offset = (Vector2){WIDTH / 2.0, HEIGHT
    // / 2.0}, .zoom = 1.0f};
    gameData.camera = (Camera2D){.target = (Vector2){0.0f, 0.0f},
                                 .offset = (Vector2){GetRenderWidth() / 2.0f, GetRenderHeight() / 2.0f},
                                 .zoom = 1.0f};
    gameData.timer = 0.0f;

    // INTERACTION //
    gameData.placing = NewCell(FLUID, WATER, 1);
    gameData.paused = true;
}

static void startup() {
    // Tell the window to use vsync and work on high DPI displays
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

    SetTraceLogCallback(CustomLog);

    InitWindow(WIDTH, HEIGHT, "Cellular Alpha 0.0.1");
    SetTargetFPS(60);

    scenePtr = &titleScene;

    // initGameData();
}

static void loop() {
    if (scenePtr == NULL) {
        LogMessage(LOG_ERROR, "Initial scene pointer NULL.");
        return;
    }

    Scene *scene = scenePtr;
    scene->init();
    while (!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE)) {

        scene->update();

        BeginDrawing();

        ClearBackground(BLACK);

        scene->draw();

        DrawFPS(WIDTH - 80, HEIGHT - 30);
        EndDrawing();

        if (scene != scenePtr) {
            LogMessage(LOG_DEBUG, "Unloading scene: %s.", scene->name);
            scene->unload();
            LogMessage(LOG_DEBUG, "Unloading sucessfull.", scene->name);
            scene = scenePtr;
            if (scene == NULL) {
                break;
            } else {
                LogMessage(LOG_DEBUG, "Loading scene: %s.", scene->name);
                scene->init();
            }
        }
    }

    if (scene != NULL) {
        scene->unload();
    }
}

static void cleanup() {}

// MAIN

int main() {
    startup();
    loop();
    cleanup();

    CloseWindow();
    return 0;
}
