// Estimating value off PI using Monte Carlo's Method

#include <stdio.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"
#include "rayutl.h"

#define WIN_DIM 700
#define POINT_SIZE 2
#define FPS 200

const int CENTER = WIN_DIM / 2;
const int R2 = WIN_DIM * WIN_DIM / 4;

RenderTexture2D screen;
unsigned int inside_count = 0;
unsigned int all_count = 0;

void update_draw_frame() {
    char text[13];
    int x = GetRandomValue(0, WIN_DIM);
    int y = GetRandomValue(0, WIN_DIM);
    Color color = RED;

    int dx = CENTER - x;
    int dy = CENTER - y;
    if ((dx * dx + dy * dy) <= R2) {
        color = GREEN;
        inside_count++;
    }
    all_count++;

    double pi = 4.0 * inside_count / all_count;
    sprintf(text, "PI: %lf", pi);

    BeginTextureMode(screen);
    DrawCircle(x, y, POINT_SIZE, color);
    EndTextureMode();

    // I'd have flickering issues with two buffers unless
    // I clear background every frame and use a texture to keep old points
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexture(screen.texture, 0, 0, WHITE);
    DrawRing((Vector2){CENTER, CENTER}, CENTER - 1, CENTER, 0, 360, 64, WHITE);
    DrawText(text, 10, 10, 20, RAYWHITE);
    EndDrawing();
}

int main() {
    SetRandomSeed(time(NULL));
    InitWindow(WIN_DIM, WIN_DIM, "Monte Carlo Pi");
    screen = LoadRenderTexture(WIN_DIM, WIN_DIM);

    rayutl_mainloop(update_draw_frame, FPS);

    CloseWindow();
    UnloadRenderTexture(screen);
    return 0;
}