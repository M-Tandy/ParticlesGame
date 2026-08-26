/**
 ********************************************************************************
 * @file     control.c
 * @author   Matthew Tandy
 * @created  2026-08-13
 * @brief    User control.
 ********************************************************************************
 */

#include "control.h"

// INCLUDES

// EXTERN VARIABLES

// PRIVATE MACROS AND DEFINES

// PRIVATE TYPEDEFS

// STATIC VARIABLES

// GLOBAL VARIABLES

// STATIC FUNCTION PROTOTYPES

// STATIC FUNCTIONS

// GLOBAL FUNCTIONS
Vector2 HighDpiWorldMousePos(Camera2D camera) {
    Vector2 mousePos = GetMousePosition();

    // Calculate Retina scaling factor
    float scaleX = (float)GetRenderWidth() / GetScreenWidth();
    float scaleY = (float)GetRenderHeight() / GetScreenHeight();

    Vector2 scaledMouse = {mousePos.x * scaleX, mousePos.y * scaleY};

    return GetScreenToWorld2D(scaledMouse, camera);
}
