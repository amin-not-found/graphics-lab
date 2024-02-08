// Conway's Game Of Life
// Ideas for future:
// - Implement patterns

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raylib.h"
#define UTL_IMPLEMENTATION
#include "utl.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

#define PANEL_H 70
#define FPS 60
// #define CIRCLE_BRUSH
#define SQUARE_BRUSH
// #define DEBUG

typedef enum {
    GridWBox,
    GridHBox,
    InputBoxCount,
} InputBox;

typedef struct {
    char *items;
    size_t capacity;
    size_t count;
} Grid;

int screen_width = 900;
int screen_height = 600;
int gridW = 450;
int grid_h = 300;
float cell_width;
float cell_height;

Grid grid;
Grid buffer;

#define index2d(pointer, x, y) ((pointer) + ((y) * gridW + (x)))

void recalculate_cell_size(int containerWidth, int containerHeight) {
    cell_width = (float)containerWidth / gridW;
    cell_height = (float)containerHeight / (grid_h);
#ifdef DEBUG
    utl_log(, UTL_DEBUG "Set Cell Size - cw: %f, ch: %f\n", cellWidth,
            cellHeight);
#endif
}

int neighbors_count(int x, int y) {
    int count = 0;
    for (int py = y - 1; py <= y + 1; py++) {
        for (int px = x - 1; px <= x + 1; px++) {
            // wrap around grid considering negative values
            count += *index2d(buffer.items, safeWrap(px, gridW),
                              safeWrap(py, grid_h));
        }
    }
    return count - *index2d(grid.items, x, y);
}

void paint_cell(int x, int y) {
    DrawRectangle((int)(x * cell_width), (int)(y * cell_height) + PANEL_H,
                  // using  ceilf for consistency when gridW % cellWidth != 0
                  (int)ceilf(cell_width), (int)ceilf(cell_height),
                  *index2d(grid.items, x, y) ? WHITE : BLACK);
}

void paint(int x, int y, int brushSize, bool useEraser) {
    brushSize = brushSize - 1;
    if (x < 0 || x >= gridW || y < 0 || y >= grid_h) return;
#ifdef CIRCLE_BRUSH
    for (int x_offset = -brushSize; x_offset <= brushSize; x_offset++) {
        for (int y_offset = -brushSize; y_offset <= brushSize; y_offset++) {
            // Check if offset position is in brushSize radius
            if (x_offset * x_offset + y_offset * y_offset <=
                brushSize * brushSize) {
                // wrap around grid considering negative values
                int py = wrap(y_offset + y, gridH);
                int px = wrap(x_offset + x, gridW);
                *index2d(grid, px, py) = !useEraser;
                paintCell(px, py);
            }
        }
    }
#endif
#ifdef SQUARE_BRUSH
    int start_x = x - brushSize / 2;
    int start_y = y - brushSize / 2;
    for (int y = start_y; y <= start_y + brushSize; y++) {
        for (int x = start_x; x <= start_x + brushSize; x++) {
            int py = safeWrap(y, grid_h);
            int px = safeWrap(x, gridW);
            *index2d(grid.items, px, py) = !useEraser;
            paint_cell(px, py);
        }
    }
#endif
}

void iterate_board() {
    memcpy(buffer.items, grid.items, grid.count * sizeof(grid.items[0]));
    for (int y = 0; y < grid_h; y++) {
        for (int x = 0; x < gridW; x++) {
            int neighbors = neighbors_count(x, y);

            if (*index2d(grid.items, x, y)) {
                if (neighbors < 2 || neighbors > 3) {
                    *index2d(grid.items, x, y) = false;
                }
            } else if (neighbors == 3) {
                *index2d(grid.items, x, y) = true;
            }
        }
    }
}

void init_grid() {
    srand(time(0));
    for (int y = 0; y < grid_h; y++) {
        for (int x = 0; x < gridW; x++) {
            // not using rand()%2 as the lower order bits are much
            // less random than the upper order bits in a LCG.
            *index2d(grid.items, x, y) = rand() > (RAND_MAX / 2);
        }
    }
}

void clear_grid() {
    for (int y = 0; y < grid_h; y++) {
        for (int x = 0; x < gridW; x++) {
            *index2d(grid.items, x, y) = 0;
        }
    }
}

// Returns non zero value on error
int resize_grid(int newGridW, int newGridH) {
    grid_h = newGridH;
    gridW = newGridW;
    // making sure new grid sizes aren't out of boundary
    if (gridW < 1) gridW = 1;
    if (grid_h < 1) grid_h = 1;
    if (gridW > screen_width) gridW = screen_width;
    if (grid_h > screen_height) grid_h = screen_height;

    int size = gridW * grid_h * sizeof(grid);
    utl_da_resize(grid, size);
    utl_da_resize(buffer, size);
    if (grid.items == NULL || buffer.items == NULL) {
        utl_log(UTL_ERROR, "Could'nt reallocate memory for grid or buffer!");
        exit(-1);
    };
    recalculate_cell_size(screen_width, screen_height - PANEL_H);
    return 0;
}

int main(void) {
    int new_grid_w = gridW;
    int new_grid_h = grid_h;
    int grid_update_pos = 0;
    int update_delay_rate = 10;
    int brush_size = 1;
    bool paused = false;

    bool grid_w_box = false;
    bool grid_h_box = false;
    bool use_eraser = false;
    bool pause_btn = false;
    bool eraser_btn = false;
    bool next_step_btn = false;
    bool randomize_btn = false;
    bool clear_btn = false;
    enum InputBox active_input_box = 0;

    utl_da_init(grid, grid_h * gridW);
    utl_da_init(buffer, grid.count);
    if (grid.items == NULL || buffer.items == NULL) {
        utl_log(UTL_ERROR, "Could'nt allocate memory for grid or buffer!");
        return -1;
    };

    init_grid();
    recalculate_cell_size(screen_width, screen_height - PANEL_H);

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screen_width, screen_height, "Conway's Game of Life");
    SetTargetFPS(FPS);

    while (!WindowShouldClose()) {
        screen_width = GetScreenWidth();
        screen_height = GetScreenHeight();
        grid_update_pos = (grid_update_pos + 1) % update_delay_rate;

        // Handling input
        if (IsKeyPressed(KEY_E) || eraser_btn) use_eraser = !use_eraser;
        if (IsKeyPressed(KEY_R) || randomize_btn) init_grid();
        if (IsKeyPressed(KEY_N) || next_step_btn) iterate_board();
        if (IsKeyPressed(KEY_SPACE) || pause_btn) paused = !paused;
        if (IsKeyPressed(KEY_C) || clear_btn) clear_grid();
        if (grid_w_box && grid_h_box)
            active_input_box = (active_input_box + 1) % InputBoxCount;
        if (IsKeyPressed(KEY_UP))
            active_input_box = (active_input_box + 1) % InputBoxCount;
        if (IsKeyPressed(KEY_DOWN))
            active_input_box = safeWrap(active_input_box - 1, InputBoxCount);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse_pos = GetMousePosition();
            int x = mouse_pos.x / cell_width;
            int y = (mouse_pos.y - PANEL_H) / cell_height;
            paint(x, y, brush_size, use_eraser);
        }
        if (IsKeyPressed(KEY_ENTER) || IsWindowResized()) {
            resize_grid(new_grid_w, new_grid_h);
        }
        if (!paused && !grid_update_pos) iterate_board();

        BeginDrawing();
        {
            ClearBackground(BLACK);
            // Draw cells
            for (int y = 0; y < grid_h; y++) {
                for (int x = 0; x < gridW; x++) {
                    paint_cell(x, y);
#ifdef DEBUG
                    char *text[10];
                    sprintf(text, "%d", neighborsCount(x, y));
                    DrawText(text, (int)(x * cellWidth) + 5,
                             (int)(y * cellHeight) + PANEL_H + 5, cellWidth / 3,
                             RED);
#endif
                }
            }

            // Draw Gui Panel Background
            DrawRectangle(0, 0, screen_width, PANEL_H, BEIGE);

            // Draw Brush & Eraser buttons for choosing mouse behaviour
            int x_offset = screen_width * 0.01;
            int width = screen_width * 0.12;
            if (use_eraser) GuiSetState(STATE_FOCUSED);
            eraser_btn = GuiButton((Rectangle){x_offset, 6, width, 58},
                                   "Use\nEraser (E)");
            GuiSetState(STATE_NORMAL);

            // Draw randomize and clear button
            x_offset = screen_width * 0.02 + width;
            width = screen_width * 0.15;
            randomize_btn = GuiButton((Rectangle){x_offset, 3, width, 32},
                                      "Randomize Grid (R)");
            clear_btn = GuiButton((Rectangle){x_offset, 33, width, 32},
                                  "Clear Grid (C)");

            // Draw Pause and Next step button
            width = screen_width * 0.18;
            x_offset = (screen_width - width) / 2.6;
            next_step_btn =
                GuiButton((Rectangle){x_offset, 5, width, 30}, "Next Step (N)");
            if (paused) GuiSetState(STATE_FOCUSED);
            pause_btn =
                GuiButton((Rectangle){x_offset + width + 5, 5, width, 30},
                          "Pause (Space)");
            GuiSetState(STATE_NORMAL);

            // Draw spinners for slowness and brush size
            GuiSpinner(
                (Rectangle){x_offset + width / 4 + 20, 40, width / 2, 20},
                "Slowness: ", &update_delay_rate, 1, FPS, false);
            GuiSpinner(
                (Rectangle){x_offset + width * 4 / 3 + 20, 40, width / 2, 20},
                "BrushSize: ", &brush_size, 1, FPS, false);

            // Draw spinners for controlling grid size
            width = screen_width * 0.2;
            x_offset = screen_width - (width + screen_width * 0.01);
            grid_w_box =
                GuiValueBox((Rectangle){x_offset, 10, width, 20},
                            "Grid Width: ", &new_grid_w, 1, screen_width,
                            active_input_box == GridWBox);
            grid_h_box =
                GuiValueBox((Rectangle){x_offset, 40, width, 20},
                            "Grid Height: ", &new_grid_h, 1, screen_width,
                            active_input_box == GridHBox);
        }
        EndDrawing();
    }

    utl_da_free(buffer);
    utl_da_free(grid);
    CloseWindow();

    return 0;
}