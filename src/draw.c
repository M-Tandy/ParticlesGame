#include "draw.h"

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
