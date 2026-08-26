#ifndef cellular_data_h
#define cellular_data_h

#include "ui.h"
#include "value.h"

typedef enum {
    TITLE,
    GRID,
    QUADTREE,
} SceneName;

typedef struct Scene {
    void (*init)(void);
    void (*update)(void);
    void (*draw)(void);
    void (*unload)(void);

    // Useful for debugging purposes
    char* name;
} Scene;

typedef struct GameData {
    SceneName sceneName;
    Scene *scene;

    Camera2D camera;
    float timer;

    Button buttonStart;
    Button buttonNextMaterial;
    Button buttonIncrementState;
    Button buttonDecrementState;

    Cell placing;
    bool paused;
} GameData;

#endif // cellular_data_h
