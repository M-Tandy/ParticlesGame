/**
 ********************************************************************************
 * @file     grid.c
 * @author   Matthew Tandy
 * @created  2026-08-26
 * @brief    Ceullular automota grid.
 ********************************************************************************
 */

// INCLUDES
#include "grid.h"

#include "common.h"
#include "control.h"
#include "data.h"
#include "debug.h"
#include "memory.h"
#include "value.h"

#include <math.h>
#include <raylib.h>

// EXTERN VARIABLES

// PRIVATE MACROS AND DEFINES

// PRIVATE TYPEDEFS
typedef struct CellNeighbourhood {
    Cell *nw;
    Cell *n;
    Cell *ne;
    Cell *w;
    Cell *c;
    Cell *e;
    Cell *sw;
    Cell *s;
    Cell *se;
} CellNeighbourhood;

// STATIC VARIABLES
static Grid grid1;
static Grid grid2;

static int gridWidthScale = GRID_RENDER_WIDTH / GRID_WIDTH;
static int gridHeightScale = GRID_RENDER_HEIGHT / GRID_WIDTH;
static int gridx = -(GRID_RENDER_WIDTH / GRID_WIDTH) * GRID_WIDTH / 2;
static int gridy = -(GRID_RENDER_HEIGHT / GRID_WIDTH) * GRID_WIDTH / 2;
static int gridWidth = GRID_WIDTH;
static RenderTexture2D gridTexture;

static Camera2D camera;

static Cell palette;

static Button buttonIncrementState;
static Button buttonDecrementState;
static Button buttonNextMaterial;

static float timer = 0.0f;
static float mouseTimer = 0.0f;
static bool paused = true;
static bool step = false;

static Cell boundary = (Cell){.type = SOLID, .material = STONE, .state = 1.0, .settled = true};

static UI ui;
// GLOBAL VARIABLES

// STATIC FUNCTION PROTOTYPES

// STATIC FUNCTIONS

/* Initialise `grid` with size `rows` x `cols` */
static void initGrid(Grid *grid, uint16_t rows, uint16_t cols) {
    grid->rows = rows;
    grid->cols = cols;

    grid->cells = ALLOCATE(Cell *, rows);
    for (int row = 0; row < rows; row++) {
        grid->cells[row] = ALLOCATE(Cell, cols);
        for (int col = 0; col < cols; col++) {
            InitCell(&grid->cells[row][col], VACUUM, NONE, 0);
        }
    }
}

static void freeGrid(Grid *grid) {
    for (int row = 0; row < grid->rows; row++) {
        FREE(Cell, grid->cells[row]);
    }

    FREE(Cell *, grid->cells);
}

static void copyGrid(const Grid *grid, Grid *result) {
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            CopyCell(&grid->cells[row][col], &result->cells[row][col]);
        }
    }
}

// Returns the cell at position `row` and `col` in grid. Returns `NULL` if the location is outside the grid.
static Cell *getCell(const Grid *grid, int row, int col) {
    if ((0 <= row) && (row <= grid->rows - 1) && (0 <= col) && (col <= grid->cols - 1)) {
        return &grid->cells[row][col];
    }
    return NULL;
}

/* Returns the cell at position `x` and `y` in the world, and stores it in `result`.
 * Returns false if location was outside the grid. In such a case `result` is unchanged. */
static bool getCellAt(const Grid *grid, float x, float y, Cell **result) {
    int row = floor((y - gridy) / gridHeightScale);
    int col = floor((x - gridx) / gridWidthScale);
    if (0 <= row && row < grid->rows && 0 <= col && col < grid->cols) {
        *result = &grid->cells[row][col];
        return true;
    }
    return false;
}

static CellNeighbourhood newCellNeighbourhood(Cell *nw, Cell *n, Cell *ne, Cell *w, Cell *c,
                                       Cell *e, Cell *sw, Cell *s, Cell *se) {
    return (CellNeighbourhood){// clang-format off
        .nw = nw,
        .n = n,
        .ne = ne,
        .w = w,
        .c = c,
        .e = e,
        .sw = sw,
        .s = s,
        .se = se
    }; // clang-format on
}

// Gets the cell neighbourhood of cell at `row` and `col`. `boundary` determines what objects outside the grid
// default to.
static CellNeighbourhood getCellNeighbourhood(const Grid *grid, int row, int col, Cell *boundary) {
    Cell *nw = getCell(grid, row - 1, col - 1);
    Cell *n = getCell(grid, row - 1, col);
    Cell *ne = getCell(grid, row - 1, col + 1);
    Cell *w = getCell(grid, row, col - 1);
    Cell *c = getCell(grid, row, col);
    Cell *e = getCell(grid, row, col + 1);
    Cell *sw = getCell(grid, row + 1, col - 1);
    Cell *s = getCell(grid, row + 1, col);
    Cell *se = getCell(grid, row + 1, col + 1);

#define IFNULL(ptr) (ptr == NULL ? boundary : ptr)

    return newCellNeighbourhood(IFNULL(nw), IFNULL(n), IFNULL(ne), IFNULL(w), IFNULL(c), IFNULL(e), IFNULL(sw),
                                IFNULL(s), IFNULL(se));
#undef IFNULL
}

static void unsettle(CellNeighbourhood n) {
    n.nw->settled = false;
    n.n->settled = false;
    n.ne->settled = false;
    n.w->settled = false;
    n.c->settled = false;
    n.e->settled = false;
    n.sw->settled = false;
    n.s->settled = false;
    n.se->settled = false;
}

static void map(Grid *grid, Grid *result) {
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            CellNeighbourhood n = getCellNeighbourhood(grid, row, col, &boundary);
        }
    }
}

#define MIN_FLOW 0.5
#define SMOOTH(x) x > MIN_FLOW ? 0.5 * x : x

static const double maxMass = 1.0;
static const double maxCompress = 0.02;
static const double minMass = 0.00001;
static const double maxSpeed = 0.5;

// Returns the maximum amount that `dest` can hold when flowing from source to destination
static double maxFlowBelow(double source, double dest) {
    double total = source + dest;

    if (total <= maxMass)
        return maxMass;

    if (total < 2 * maxMass + maxCompress) {
        return (maxMass * maxMass + total * maxCompress) / (maxMass + maxCompress);
    }

    return (total + maxCompress) / 2.0;
}

// Evolves `grid` and stores it in `result`
static void evolve(const Grid *grid, Grid *result) {
    double flow = 0.0;
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            Cell cell = grid->cells[row][col];

            if (cell.type != FLUID)
                continue;

            flow = 0.0;
            double remainingMass = cell.state;

            if (remainingMass <= 0.0) {
                result->cells[row][col].type = VACUUM;
                result->cells[row][col].material = NONE;
                result->cells[row][col].settled = NONE;
                continue;
            }

            CellNeighbourhood n = getCellNeighbourhood(grid, row, col, &boundary);
            CellNeighbourhood n_r = getCellNeighbourhood(result, row, col, &boundary);

            // Below
            if (n.s->type != SOLID) {
                flow = maxFlowBelow(n.c->state, n.s->state) - n.s->state;
                flow = SMOOTH(flow);
                flow = CONSTRAIN(flow, 0, MIN(remainingMass, maxSpeed));

                if (flow >= 0.001) {
                    n_r.c->state -= flow;
                    n_r.s->state += flow;
                    n_r.s->type = FLUID;
                    n_r.s->material = WATER;
                    n_r.s->settled = false;
                    remainingMass -= flow;
                }
            }

            if (remainingMass <= 0.0)
                continue;

            // LEFT
            if (n.w->type != SOLID) {
                flow = (n.c->state - n.w->state) / 4.0;
                flow = SMOOTH(flow);
                flow = CONSTRAIN(flow, 0, remainingMass);

                if (flow >= 0.001) {
                    n_r.c->state -= flow;
                    n_r.w->state += flow;
                    n_r.w->type = FLUID;
                    n_r.w->material = WATER;
                    n_r.w->settled = false;
                    remainingMass -= flow;
                }
            }

            if (remainingMass <= 0.0)
                continue;

            // RIGHT
            if (n.e->type != SOLID) {
                flow = (n.c->state - n.e->state) / 4.0;
                flow = SMOOTH(flow);
                flow = CONSTRAIN(flow, 0, remainingMass);

                if (flow >= 0.001) {
                    n_r.c->state -= flow;
                    n_r.e->state += flow;
                    n_r.e->type = FLUID;
                    n_r.e->material = WATER;
                    n_r.e->settled = false;
                    remainingMass -= flow;
                }
            }

            if (remainingMass <= 0.0)
                continue;

            // UP
            if (n.n->type != SOLID) {
                flow = remainingMass - maxFlowBelow(n.n->state, remainingMass);
                flow = SMOOTH(flow);
                flow = CONSTRAIN(flow, 0, remainingMass);

                if (flow >= 0.001) {
                    n_r.c->state -= flow;
                    n_r.n->state += flow;
                    n_r.n->type = FLUID;
                    n_r.n->material = WATER;
                    n_r.n->settled = false;
                    remainingMass -= flow;
                }
            }

            n_r.c->settled = false;
        }
    }
}

#undef MIN_FLOW
#undef SMOOTH

static void evolveGrid(const Grid *grid, Grid *result) {
    if (grid->rows != result->rows || grid->cols != result->cols) {
        LogMessage(LOG_ERROR, "grid %p and result %p have misaligned columns or rows: grid (%d, %d), result (%d, %d)",
                   grid, result, grid->rows, grid->cols, result->rows, result->cols);
    }

    copyGrid(grid, result);
    evolve(grid, result);
}

static void paletteNext() {
    switch (palette.material) {

    case NONE:
        palette.type = GAS;
        palette.material = AIR;
        break;
    case AIR:
        palette.type = FLUID;
        palette.material = WATER;
        break;
    case WATER:
        palette.type = FLUID;
        palette.material = LAVA;
        break;
    case LAVA:
        palette.type = SOLID;
        palette.material = STONE;
        break;
    case STONE:
        palette.type = VACUUM;
        palette.material = NONE;
        break;
    }
}

static void paletteStronger() {
    palette.state = CONSTRAIN(palette.state + 0.1, 0.0, 1.0); 
}

static void paletteWeaker() {
    palette.state = CONSTRAIN(palette.state - 0.1, 0.0, 1.0); 
}

static void init() {
    LogMessage(LOG_DEBUG, "Initialising grid scene.");

    LogMessage(LOG_DEBUG, "Initialising first grid");
    initGrid(&grid1, GRID_WIDTH, GRID_WIDTH);
    LogMessage(LOG_DEBUG, "Initialising second grid");
    initGrid(&grid2, GRID_WIDTH, GRID_WIDTH);

    LogMessage(LOG_DEBUG, "Loading render texture");
    gridTexture = LoadRenderTexture(GRID_WIDTH, GRID_WIDTH);

    LogMessage(LOG_DEBUG, "Setting camera");
    camera = (Camera2D){.target = (Vector2){0.0f, 0.0f},
                        .offset = (Vector2){GetRenderWidth() / 2.0f, GetRenderHeight() / 2.0f},
                        .zoom = 1.0f};

    LogMessage(LOG_DEBUG, "Defining pallette");
    palette.type = FLUID;
    palette.material = WATER;
    palette.state = 0.5;

    LogMessage(LOG_DEBUG, "Grid UI");
    initUI(&ui);

    LogMessage(LOG_DEBUG, "Creating buttons");
    addButton(&ui, (Rectangle){WIDTH - 50, 200, 50, 50}, true, "->", 32, paletteNext);
    addButton(&ui, (Rectangle){WIDTH - 50, 300, 50, 50}, true, "+", 32, paletteStronger);
    addButton(&ui, (Rectangle){WIDTH - 50, 400, 50, 50}, true, "-", 32, paletteWeaker);

    LogMessage(LOG_DEBUG, "Grid scene initialised.");

    // Prevent mouse events for a short amount of time
    mouseTimer = 0.0f;
};

static void updateCamera(Camera2D *camera) {
    if (IsKeyDown(KEY_W)) {
        camera->target.y -= CAMERA_SPEED * camera->zoom;
    }
    if (IsKeyDown(KEY_S)) {
        camera->target.y += CAMERA_SPEED * camera->zoom;
    }
    if (IsKeyDown(KEY_D)) {
        camera->target.x += CAMERA_SPEED * camera->zoom;
    }
    if (IsKeyDown(KEY_A)) {
        camera->target.x -= CAMERA_SPEED * camera->zoom;
    }

    if (IsKeyDown(KEY_UP)) {
        camera->offset.y += CAMERA_SPEED * camera->zoom;
    }
    if (IsKeyDown(KEY_DOWN)) {
        camera->offset.y -= CAMERA_SPEED * camera->zoom;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        camera->offset.x -= CAMERA_SPEED * camera->zoom;
    }
    if (IsKeyDown(KEY_LEFT)) {
        camera->offset.x += CAMERA_SPEED * camera->zoom;
    }

    if (IsKeyPressed(KEY_O)) {
        if (camera->zoom > 1.0f) {
            camera->zoom -= 1.0f;
        } else {
            camera->zoom /= 2.0f;
        }
    }
    if (IsKeyPressed(KEY_P) && camera->zoom < 8.0f) {
        if (camera->zoom > 1.0f) {
            camera->zoom += 1.0f;
        } else {
            camera->zoom *= 2.0f;
        }
    }
}

static void updateTimer() {
    float dt = GetFrameTime();
    timer += dt;

    if (mouseTimer < 1.0f) {
        mouseTimer += dt;
    }
}

static void mouseInput() {
    if (mouseTimer < 0.9f) {
        return;
    }

    MouseButton mouseButton;
    if (mouseDown(&mouseButton)) {
        // Works without high-dpi mode
        // Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData->camera);

        Vector2 worldPos = HighDpiWorldMousePos(camera);

        Cell *result = NULL;
        if (mouseButton == MOUSE_BUTTON_LEFT) {
            if (getCellAt(&grid1, worldPos.x, worldPos.y, &result)) {
                *result = palette;
                result->settled = false;
            }
        } else if (mouseButton == MOUSE_BUTTON_RIGHT) {
            if (getCellAt(&grid1, worldPos.x, worldPos.y, &result)) {
                result->material = NONE;
                result->type = VACUUM;
                result->state = 0;
            }
        }
    }
}

static void keyboardInput() {
    if (IsKeyPressed(KEY_SPACE)) {
        paused = !paused;
    }
    if (paused && IsKeyPressed(KEY_PERIOD)) {
        step = true;
    }
}

static void updateGrids() {
    if ((!paused && timer > 1.0f / (float)UPDATE_RATE) || step) {
        evolveGrid(&grid1, &grid2);
        SWAP(grid1, grid2, Grid);
        timer = 0.0f;
        step = false;
    }
}

static void update() {
    updateTimer();
    updateCamera(&camera);

    updateUI(ui);

    mouseInput();
    keyboardInput();

    updateGrids();
}

static void drawGridPixels(const Grid *grid, int x, int y) {
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            if (!grid->cells[row][col].settled) {
                DrawPixel(x + col, y + row, CellColor(grid->cells[row][col]));
                grid->cells[row][col].settled = true;
            }
        }
    }
}

static void drawGridUI() {
    // Selected Cell UI
    DrawCell(palette, WIDTH - 100, 50, 50, 50);
    DrawText(TextFormat("%f", palette.state), WIDTH - 90, 60, 32, WHITE);

    drawUI(ui);
}

static void drawDebugUI() {
    DrawText(TextFormat("%f", camera.zoom), 10, 10, 20, WHITE);
    DrawText(TextFormat("Time: %f", timer), 10, 30, 20, BLUE);
    DrawText(TextFormat("%s", paused ? "Paused" : ""), WIDTH - 200, 200, 32, RED);
    DrawText(TextFormat("Camera Target - %f, %f", camera.target.x, camera.target.y), 10, 50, 20, BLUE);
    DrawText(TextFormat("Camera Offset - %f, %f", camera.offset.x, camera.offset.y), 10, 70, 20, BLUE);
    DrawText(TextFormat("Camera Zoom - %f", camera.zoom), 10, 90, 20, BLUE);
}

static void debugInfo(const Grid *grid) {
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            Cell cell = grid->cells[row][col];
            Vector2 cellLocation = (Vector2){gridx + col * gridWidthScale, gridy + row * gridHeightScale};
            DrawText(TextFormat("%f", cell.state), cellLocation.x, cellLocation.y, 1, WHITE);
        }
    }
}

static void draw() {
    BeginTextureMode(gridTexture); // clang-format off
        drawGridPixels(&grid1, 0, 0);
    EndTextureMode(); // clang-format on

    ClearBackground(BLACK);

    BeginMode2D(camera); // clang-format off
        // Stretching gridTexture, drawn using pixels, using the gridWidthScale/grigHeightScale values.
        DrawTexturePro(gridTexture.texture,
                    (Rectangle){0, 0, (float)gridTexture.texture.width, -(float)gridTexture.texture.height},
                    (Rectangle){gridx, gridy, gridWidthScale * gridWidth, gridHeightScale * gridWidth}, (Vector2){0, 0},
                    0.0f, WHITE);

        debugInfo(&grid1);
        DebugDrawCameraTargetGuides(camera);
    EndMode2D(); // clange-format on

    drawGridUI();
    drawDebugUI();
};

static void unload() {
    LogMessage(LOG_DEBUG, "Freeing Grids");
    freeGrid(&grid1);
    freeGrid(&grid2);
};

// GLOBAL FUNCTIONS

Scene GridScene = (Scene){
    .init = init,
    .update = update,
    .draw = draw,
    .unload = unload,
    .name = "grid",
};
