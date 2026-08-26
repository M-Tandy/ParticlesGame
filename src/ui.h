#ifndef ptest_ui_h
#define ptest_ui_h

#include "common.h"
#include "raylib.h"

// Event objects are function pointers
typedef void (*Event)();

struct UIElement {
    Rectangle bounds;
    bool state;
};

typedef struct Button {
    struct UIElement element;
    char *text;
    int fontSize;
    Event onclick;
} Button;

typedef struct UI {
    Button *buttons;
    uint8_t count_buttons;
    uint8_t capacity_buttons;
} UI;

void initUI(UI *ui);
void addButton(UI *ui, Rectangle bounds, bool initialState, char *text, int fontSize, Event onclick);
void updateUI(UI ui);
void drawUI(UI ui);
void drawButton(Button button);
bool mouseDown(MouseButton *button);

#endif // ptest_ui_h
