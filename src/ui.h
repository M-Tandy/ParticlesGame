#ifndef ptest_ui_h
#define ptest_ui_h

#include "raylib.h"

// Event objects are function pointers
typedef void (*Event)();

typedef struct UIElement {
    Rectangle bounds;
    bool state;
} UIElement;

typedef struct Button {
    UIElement element;
    char *text;
    int fontSize;
    Event onclick;
} Button;

Button newButton(Rectangle bounds, bool initialState, char *text, int fontSize, Event onclick);
void drawButton(Button button);
bool tryButtonPress(Button button);

#endif // ptest_ui_h
