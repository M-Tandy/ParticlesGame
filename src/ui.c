#include "ui.h"
#include "common.h"
#include <raylib.h>

void initUIElement(Rectangle bounds, bool initialState, UIElement *ele) {
    ele->bounds = bounds;
    ele->state = initialState;
}

void drawUIElement(UIElement ele) {
    Vector2 mousePos = GetMousePosition();
    if (IN_RECT(mousePos, ele.bounds)) {
        DrawRectangleRec(ele.bounds, ele.state ? ORANGE : RED);
    } else {
        DrawRectangleRec(ele.bounds, ele.state ? GREEN : RED);
    }
}

Button newButton(Rectangle bounds, bool initialState, char *text, int fontSize, Event onclick) {
    Button button;
    initUIElement(bounds, initialState, &button.element);

    button.text = text;
    button.fontSize = fontSize;
    button.onclick = onclick;

    return button;
}

void drawButton(Button button) {
    drawUIElement(button.element);
    Vector2 textSize = MeasureTextEx(GetFontDefault(), button.text, button.fontSize, 1);
    DrawText(button.text, button.element.bounds.x + button.element.bounds.width / 2 - textSize.x / 2,
             button.element.bounds.y + button.element.bounds.height / 2 - textSize.y / 2, button.fontSize, WHITE);
}

void callButton(Button button) { button.onclick(); }

bool tryButtonPress(Button button) {
    Vector2 mousePos = GetMousePosition();
    if (IN_RECT(mousePos, button.element.bounds)) {
        callButton(button);
        return true;
    }
    return false;
}
