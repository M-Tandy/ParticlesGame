#ifndef ptest_draw_h
#define ptest_draw_h

#include "raylib.h"

void DrawCenteredSquareLines(Vector2 pos, float width, Color color);
void DrawCenteredSquare(Vector2 pos, float width, Color color);
void DrawGridUnderlay(Vector2 centre, int rows, int cols, float spacing);
#endif // ptest_draw_h
