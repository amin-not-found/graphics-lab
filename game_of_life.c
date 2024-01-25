// Conway's Game Of Life
// Ideas for future:
// - Implement patterns

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

#define PANEL_H 70
#define FPS 60
// #define CIRCLE_BRUSH
#define SQUARE_BRUSH
// #define DEBUG

enum InputBox {
  GridWBox,
  GridHBox,
  InputBoxCount,
};

int screenWidth = 900;
int screenHeight = 600;
int gridW = 90;
int gridH = 60;
float cellWidth;
float cellHeight;

char *grid;
char *buffer;

#define index2d(pointer, x, y) ((pointer) + ((y) * gridW + (x)))
int safeWrap(int value, int max) { return ((value % max) + max) % max; }

void recalculateCellSize(int containerWidth, int containerHeight) {
  cellWidth = (float)containerWidth / gridW;
  cellHeight = (float)containerHeight / (gridH);
#ifdef DEBUG
  printf("[Info] Set Cell Size - cw: %f, ch: %f\n", cellWidth, cellHeight);
#endif
}

int neighborsCount(int x, int y) {
  int count = 0;
  for (int py = y - 1; py <= y + 1; py++) {
    for (int px = x - 1; px <= x + 1; px++) {
      // wrap around grid considering negative values
      count += *index2d(buffer, safeWrap(px, gridW), safeWrap(py, gridH));
    }
  }
  return count - *index2d(grid, x, y);
}

void paintCell(int x, int y) {
  DrawRectangle((int)(x * cellWidth), (int)(y * cellHeight) + PANEL_H,
                // using  ceilf for consistency when gridW % cellWidth != 0
                (int)ceilf(cellWidth), (int)ceilf(cellHeight),
                *index2d(grid, x, y) ? WHITE : BLACK);
}

void paint(int x, int y, int brushSize, bool useErasor) {
  brushSize = brushSize - 1;
  if (x < 0 || x >= gridW || y < 0 || y >= gridH) return;
#ifdef CIRCLE_BRUSH
  for (int x_offset = -brushSize; x_offset <= brushSize; x_offset++) {
    for (int y_offset = -brushSize; y_offset <= brushSize; y_offset++) {
      // Check if offset position is in brushSize radius
      if (x_offset * x_offset + y_offset * y_offset <= brushSize * brushSize) {
        // wrap around grid considering negative values
        int py = wrap(y_offset + y, gridH);
        int px = wrap(x_offset + x, gridW);
        *index2d(grid, px, py) = !useErasor;
        paintCell(px, py);
      }
    }
  }
#endif
#ifdef SQUARE_BRUSH
  int startX = x - brushSize / 2;
  int startY = y - brushSize / 2;
  for (int y = startY; y <= startY + brushSize; y++) {
    for (int x = startX; x <= startX + brushSize; x++) {
      int py = safeWrap(y, gridH);
      int px = safeWrap(x, gridW);
      *index2d(grid, px, py) = !useErasor;
      paintCell(px, py);
    }
  }
#endif
}

void iterateBoard() {
  memcpy(buffer, grid, gridH * gridW * sizeof(grid[0]));
  for (int y = 0; y < gridH; y++) {
    for (int x = 0; x < gridW; x++) {
      int neighbors = neighborsCount(x, y);

      if (*index2d(grid, x, y)) {
        if (neighbors < 2 || neighbors > 3) {
          *index2d(grid, x, y) = false;
        }
      } else if (neighbors == 3) {
        *index2d(grid, x, y) = true;
      }
    }
  }
}

void initGrid() {
  srand(time(0));
  for (int y = 0; y < gridH; y++) {
    for (int x = 0; x < gridW; x++) {
      // not using rand()%2 as the lower order bits are much
      // less random than the upper order bits in a LCG.
      *index2d(grid, x, y) = rand() > (RAND_MAX / 2);
    }
  }
  memcpy(buffer, grid, gridH * gridW * sizeof(grid[0]));
}

void clearGrid() {
  for (int y = 0; y < gridH; y++) {
    for (int x = 0; x < gridW; x++) {
      *index2d(grid, x, y) = 0;
    }
  }
  memcpy(buffer, grid, gridH * gridW * sizeof(grid[0]));
}

// Returns non zero value on error
int resizeGrid(int newGridW, int newGridH) {
  gridH = newGridH;
  gridW = newGridW;
  // making sure new grid sizes aren't out of boundary
  if (gridW < 1) gridW = 1;
  if (gridH < 1) gridH = 1;
  if (gridW > screenWidth) gridW = screenWidth;
  if (gridH > screenHeight) gridH = screenHeight;

  int size = gridW * gridH * sizeof(grid);
  grid = realloc(grid, size);
  buffer = realloc(buffer, size);
  if (grid == NULL || buffer == NULL) {
    printf("[Error] Could'nt reallocate memory for grid or buffer!\n");
    return -1;
  };
  recalculateCellSize(screenWidth, screenHeight - PANEL_H);
  return 0;
}

int main(void) {
  int newGridW = gridW;
  int newGridH = gridH;
  int gridUpdatePos = 0;
  enum InputBox activeInputBox = 0;
  bool paused = false;

  int updateDelayRate = 10;
  int brushSize = 1;
  bool gridWBox = false;
  bool gridHBox = false;
  bool useErasor = false;
  bool pauseBtn = false;
  bool erasarBtn = false;
  bool nextStepBtn = false;
  bool randomizeBtn = false;
  bool clearBtn = false;

  grid = malloc(gridH * gridW * sizeof(grid[0]));
  buffer = malloc(gridH * gridW * sizeof(grid[0]));
  if (grid == NULL || buffer == NULL) {
    printf("Could'nt allocate memory for grid or buffer!\n");
    return -1;
  };

  initGrid();
  recalculateCellSize(screenWidth, screenHeight - PANEL_H);

  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");
  SetTargetFPS(FPS);

  while (!WindowShouldClose()) {
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();
    gridUpdatePos = (gridUpdatePos + 1) % updateDelayRate;

    // Handling input
    if (IsKeyPressed(KEY_E) || erasarBtn) useErasor = !useErasor;
    if (IsKeyPressed(KEY_R) || randomizeBtn) initGrid();
    if (IsKeyPressed(KEY_N) || nextStepBtn) iterateBoard();
    if (IsKeyPressed(KEY_SPACE) || pauseBtn) paused = !paused;
    if (IsKeyPressed(KEY_C) || clearBtn) clearGrid();
    if (gridWBox && gridHBox)  activeInputBox = (activeInputBox + 1) % InputBoxCount;
    if (IsKeyPressed(KEY_UP))
      activeInputBox = (activeInputBox + 1) % InputBoxCount;
    if (IsKeyPressed(KEY_DOWN))
      activeInputBox = safeWrap(activeInputBox - 1, InputBoxCount);
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      Vector2 mousePos = GetMousePosition();
      int x = mousePos.x / cellWidth;
      int y = (mousePos.y - PANEL_H) / cellHeight;
      paint(x, y, brushSize, useErasor);
    }
    if (IsKeyPressed(KEY_ENTER) || IsWindowResized()) {
      resizeGrid(newGridW, newGridH);
    }
    if (!paused && !gridUpdatePos) iterateBoard();

    BeginDrawing();

    ClearBackground(BLACK);
    // Draw cells
    for (int y = 0; y < gridH; y++) {
      for (int x = 0; x < gridW; x++) {
        paintCell(x, y);
#ifdef DEBUG
        char *text[10];
        sprintf(text, "%d", neighborsCount(x, y));
        DrawText(text, (int)(x * cellWidth) + 5,
                 (int)(y * cellHeight) + PANEL_H + 5, cellWidth / 3, RED);
#endif
      }
    }

    // Draw Gui Panel Background
    DrawRectangle(0, 0, screenWidth, PANEL_H, BEIGE);

    // Draw Brush & Erasar buttons for choosing mouse behaviour
    int x_offset = screenWidth * 0.01;
    int width = screenWidth * 0.12;
    if (useErasor) GuiSetState(STATE_FOCUSED);
    erasarBtn =
        GuiButton((Rectangle){x_offset, 6, width, 58}, "Use\nEraser (E)");
    GuiSetState(STATE_NORMAL);

    // Draw randomize and clear button
    x_offset = screenWidth * 0.02 + width;
    width = screenWidth * 0.15;
    randomizeBtn =
        GuiButton((Rectangle){x_offset, 3, width, 32}, "Randomize Grid (R)");
    clearBtn =
        GuiButton((Rectangle){x_offset, 33, width, 32}, "Clear Grid (C)");

    // Draw Pause and Next step button
    width = screenWidth * 0.18;
    x_offset = (screenWidth - width) / 2.6;
    nextStepBtn =
        GuiButton((Rectangle){x_offset, 5, width, 30}, "Next Step (N)");
    if (paused) GuiSetState(STATE_FOCUSED);
    pauseBtn = GuiButton((Rectangle){x_offset + width + 5, 5, width, 30},
                         "Pause (Space)");
    GuiSetState(STATE_NORMAL);

    // Draw spinners for slonewss and brush size
    GuiSpinner((Rectangle){x_offset + width / 4 + 20, 40, width / 2, 20},
               "Slowness: ", &updateDelayRate, 1, FPS, false);
    GuiSpinner((Rectangle){x_offset + width * 4 / 3 + 20, 40, width / 2, 20},
               "BrushSize: ", &brushSize, 1, FPS, false);

    // Draw spinners for controlling grid size
    width = screenWidth * 0.2;
    x_offset = screenWidth - (width + screenWidth * 0.01);
    gridWBox = GuiValueBox((Rectangle){x_offset, 10, width, 20},
                           "Grid Width: ", &newGridW, 1, screenWidth,
                           activeInputBox == GridWBox);
    gridHBox = GuiValueBox((Rectangle){x_offset, 40, width, 20},
                           "Grid Height: ", &newGridH, 1, screenWidth,
                           activeInputBox == GridHBox);
    EndDrawing();
  }

  free(grid);
  free(buffer);
  CloseWindow();

  return 0;
}