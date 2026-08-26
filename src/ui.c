/**
 ********************************************************************************
 * @file     ui.c
 * @author   Matthew Tandy
 * @created  Unknown
 * @brief    For drawing and creating new UI elements.
 ********************************************************************************
 */

// INCLUDES
#include "ui.h"

#include "debug.h"
#include "memory.h"

#include <raylib.h>

// EXTERN VARIABLES

// PRIVATE MACROS AND DEFINES

// PRIVATE TYPEDEFS
typedef struct UIElement Element;

// STATIC VARIABLES

// GLOBAL VARIABLES

// STATIC FUNCTION PROTOTYPES

// STATIC FUNCTIONS
static void initElement(Rectangle bounds, bool initialState, Element *ele) {
    ele->bounds = bounds;
    ele->state = initialState;
}

static void drawElement(Element ele) {
    Vector2 mousePos = GetMousePosition();
    if (IN_RECT(mousePos, ele.bounds)) {
        DrawRectangleRec(ele.bounds, ele.state ? ORANGE : RED);
    } else {
        DrawRectangleRec(ele.bounds, ele.state ? GREEN : RED);
    }
}

static void initButton(Button *button, Rectangle bounds, bool initialState, char *text, int fontSize, Event onclick) {
    initElement(bounds, initialState, &button->element);
    button->text = text;
    button->fontSize = fontSize;
    button->onclick = onclick;
}

static void callButton(Button button) { button.onclick(); }

static bool tryButtonPress(Button button) {
    Vector2 mousePos = GetMousePosition();
    if (IN_RECT(mousePos, button.element.bounds)) {
        callButton(button);
        return true;
    }
    return false;
}

// GLOBAL FUNCTIONS
void initUI(UI *ui) {
    ui->count_buttons = 0;
    ui->capacity_buttons = 0;
    ui->buttons = ALLOCATE(Button, 0);
}

void addButton(UI *ui, Rectangle bounds, bool initialState, char *text, int fontSize, Event onclick) {
    if (ui->count_buttons + 1 > ui->capacity_buttons) {
        int capacity = GROW_CAPACITY(ui->capacity_buttons);
        ui->buttons = GROW_ARRAY(Button, ui->buttons, ui->capacity_buttons, capacity);
        ui->capacity_buttons = capacity;
    }

    initButton(&ui->buttons[ui->count_buttons], bounds, initialState, text, fontSize, onclick);
    ui->count_buttons += 1;
}

void updateUI(UI ui) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < ui.count_buttons; i++) {
            if (tryButtonPress(ui.buttons[i]))
                break;
        }
    }
}

void drawUI(UI ui) {
    for (int i = 0; i < ui.count_buttons; i++) {
        drawButton(ui.buttons[i]);
    }
}

void drawButton(Button button) {
    drawElement(button.element);
    Vector2 textSize = MeasureTextEx(GetFontDefault(), button.text, button.fontSize, 1);
    DrawText(button.text, button.element.bounds.x + button.element.bounds.width / 2 - textSize.x / 2,
             button.element.bounds.y + button.element.bounds.height / 2 - textSize.y / 2, button.fontSize, WHITE);
}

bool mouseDown(MouseButton *button) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        *button = MOUSE_BUTTON_LEFT;
        return true;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        *button = MOUSE_BUTTON_RIGHT;
        return true;
    }

    return false;
}
