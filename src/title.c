/**
 ********************************************************************************
 * @file     title.c
 * @author   Matthew Tandy
 * @created  2026-08-13
 * @brief    Drawing and logic for the title screen.
 ********************************************************************************
 */

// INCLUDES
#include "title.h"
 
#include "common.h"
#include "grid.h"
#include "ui.h"

// EXTERN VARIABLES

// PRIVATE MACROS AND DEFINES

// PRIVATE TYPEDEFS

// STATIC VARIABLES
UI ui;
static Button buttonStart;
static Button buttonQuadTree;
static Button buttonExit;

// GLOBAL VARIABLES

// STATIC FUNCTION PROTOTYPES

// STATIC FUNCTION
static void toGridCallback() {
    scenePtr = &GridScene;
}

static void toQuadTreeCallback() {}

static void init() {
    initUI(&ui);

    addButton(&ui, (Rectangle){WIDTH / 2 - 200 / 2 - 200, HEIGHT / 2, 200, 100}, true, "Grid", 32, toGridCallback);
    addButton(&ui, (Rectangle){WIDTH / 2 - 100, HEIGHT / 2 + 200, 200, 100}, true, "Exit", 32, toGridCallback);
}

static void update() {
    updateUI(ui);
}

static void draw() {
    int textSize = MeasureText("Cellular Alpha", 64);
    DrawText("Cellular Alpha", WIDTH / 2 - textSize / 2, HEIGHT / 2 - 100, 64, WHITE);
    drawUI(ui);
}

static void unload() {}

// GLOBAL FUNCTIONS
Scene titleScene = (Scene){.init = init, .update = update, .draw = draw, .unload = unload, .name = "title"};
