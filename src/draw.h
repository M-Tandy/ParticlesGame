#ifndef ptest_draw_h
#define ptest_draw_h

#include "raylib.h"
#include "data.h"

void drawCenteredSquareLines(Vector2 pos, float width, Color color);
void drawCenteredSquare(Vector2 pos, float width, Color color);
void drawGridUnderlay(Vector2 centre, int rows, int cols, float spacing);
void draw(GameData *gameData);
#endif // ptest_draw_h
