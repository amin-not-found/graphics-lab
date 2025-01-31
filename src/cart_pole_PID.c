// Cart-Pole stabilization simulation with PID controller
// based on:
// https://underactuated.mit.edu/acrobot.html
// https://en.wikipedia.org/wiki/Proportional–integral–derivative_controller

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#define MOTION_IMPLEMENTATION
#include "motion.h"
#define UTL_IMPLEMENTATION
#include "rayutl.h"
#include "utl.h"

#define FPS 100
#define WIN_W 1400
#define WIN_H 900
#define PANEL_H (WIN_H * 0.2)
#define CART_W 100
#define CART_H 60
#define CART_Y ((WIN_H + CART_H) / 2)
#define DAMP_COEFF 0
#define SPINNER_W 90
#define SPINNER_H 30
#define SPINNER_SPACING WIN_W / 12
#define SPINNER_MARGIN WIN_W / 5
#define MAX_INPUT INT_MAX

typedef enum {
    NONE,
    HELP,
    X_P,
    X_I,
    X_D,
    TH_P,
    TH_I,
    TH_D,
} UIState;

typedef struct {
    // the x component is the x component but the y component is theta
    Vector2 r;
    Vector2 v;
    Vector2 a;
} System;

/* Declarations and definitions */
System sys;
Vector2 forces = {0, 2e3};  // Fx and gravity
double cart_mass = 25;
double pole_mass = 50;
double pole_length = 200;

int x_p = 100;
int x_i = 0;
int x_d = 120;
int theta_p = 700000;
int theta_i = 250;
int theta_d = 60000;
double x_error_prev = 0;
double x_error_sum = 0;
double theta_error_prev = 0;
double theta_error_sum = 0;

UIState ui_state = NONE;
bool paused = false;
bool dragging = false;
Particle *clicked_node = NULL;

const int HELP_X = WIN_W - 50;
const int HELP_Y = 36;
const int HELP_FONT = 27;
Rectangle help_rect;

const char *help_text =
    //"You can use your cursor to drag the cart or the pole. \n\n\n\n" // TODO
    "Cart-Pole system simulation with PID controller. \n\n\n\n"
    " \n\n\n\n"
    "You can change controller parameters for both \n\n\n\n"
    "angular and horizontal movement. \n\n\n\n"
    " \n\n\n\n"
    "Space: pause the simulation \n\n\n\n"
    "Enter: advance simulation one step \n\n\n\n"
    " \n\n\n\n"
    "Click anywhere to resume. \n\n\n\n";

/* End of declarations */

// Does a linear search though all particles.
// Only is performant for not too large numbers of particles
// Returns null if there's no particle in a radius of distance/2
Particle *nearest_particle(Particle nodes[], Vector2 pos) {
    float least_distance = WIN_W / 500;
    Particle *result = NULL;
    for (size_t i = 0; i < 2; i++) {
        float distance = Vector2Length(Vector2Subtract(nodes[i].r, pos));
        if (distance <= least_distance) {
            least_distance = distance;
            result = nodes + i;
        }
    }
    return result;
}

void update_physics(double dt) {
    utl_log(UTL_DEBUG, "---");
    utl_log(UTL_DEBUG, "f_x : %f", forces.x);

    // based on https://underactuated.mit.edu/acrobot.html
    // angle's reference line is vertical line from bottom(clockwise)
    sys.r.y = util_wrap_angle(sys.r.y);
    double x_error = (sys.r.x - WIN_W / 2);
    double theta_error = (PI - sys.r.y);
    x_error_sum += x_error * dt;
    theta_error_sum += theta_error * dt;

    forces.x = 0;
    forces.x +=
        x_p * x_error + x_d * (x_error - x_error_prev) / dt + x_i * x_error_sum;
    forces.x += theta_p * theta_error +
        theta_d * (theta_error - theta_error_prev) / dt +
        theta_i * theta_error_sum;
    x_error_prev = x_error;
    theta_error_prev = theta_error;

    utl_log(UTL_DEBUG, "e_th: %f, f_x : %f", theta_error, forces.x);

    System new_sys = sys;
    double sin_th = sinl(sys.r.y);
    double cos_th = cosl(sys.r.y);
    double inertia = cart_mass + pole_mass * powf(sin_th, 2);

    new_sys.a.x += 1 / inertia *
        (forces.x +
         pole_mass * sin_th *
             (pole_length * pow(sys.v.y, 2) + forces.y * cos_th));
    new_sys.a.y += 1 / (inertia * pole_length) *
        (-forces.x * cos_th -
         pole_mass * pole_length * pow(sys.v.y, 2) * cos_th * sin_th -
         (pole_mass + cart_mass) * forces.y * sin_th);

    new_sys.a.x -= DAMP_COEFF * sys.v.x;
    new_sys.a.y -= DAMP_COEFF * sys.v.y;

    // integrate the link position
    mot_adaptive_rk4(&new_sys.r, &new_sys.v, new_sys.a, sys.a, dt);

    sys = new_sys;
    sys.a = Vector2Zero();
}

void update_draw_frame(void) {
    // Handle input
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;
    if (IsKeyPressed(KEY_ENTER)) {
        update_physics(1 / (double)FPS);
        ui_state = NONE;
    }

    Vector2 mouse_pos = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        // Set clicked_particle before dragging starts
        // so it doesn't change with mouse pos
        // if (!dragging) {
        //     clicked_node = nearest_particle(nodes, mouse_pos);
        // }
        // Initiate dragging if mouse position delta is big enough
        if (Vector2Length(GetMouseDelta()) > 1.0 && clicked_node != NULL) {
            dragging = true;
        }
        // Update particle position based on mouse position
        if (dragging) {
            Vector2 drag_acc =
                Vector2Scale(Vector2Subtract(mouse_pos, clicked_node->r), 5.0);
            mot_integrate_verlet(
                &clicked_node->r, drag_acc, clicked_node->r, 0.1, 0
            );
        }
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (dragging) {
            dragging = false;
        } else if (CheckCollisionPointRec(mouse_pos, help_rect)) {
            ui_state = HELP;
        }
    }

    // Show help dialog
    if (ui_state == HELP) {
        BeginDrawing();

        DrawRectangle(100, 50, WIN_H - 200, WIN_W - 100, LIGHTGRAY);
        DrawText(help_text, 170, 130, 24, BLACK);
        EndDrawing();

        if (IsKeyReleased(KEY_ENTER) ||
            IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            ui_state = NONE;
        }
        return;
    }

    if (!paused) update_physics(1 / (double)FPS);

    Rectangle cart = {sys.r.x - CART_W / 2, CART_Y, CART_W, CART_H};
    Vector2 pole_start = {sys.r.x, CART_Y + CART_H / 2};
    Vector2 pole_end = {
        pole_start.x + pole_length * sinl(sys.r.y),
        pole_start.y + pole_length * cosl(sys.r.y)
    };

    BeginDrawing();
    {
        ClearBackground(BLACK);

        // Helper lines to see how far from center the cart is
        DrawLine(0, CART_Y + CART_H / 2, WIN_W, CART_Y + CART_H / 2, DARKGREEN);
        DrawLine(WIN_W / 2, 0, WIN_W / 2, WIN_H, DARKGREEN);

        DrawRectangle(0, 0, WIN_W, PANEL_H, MAROON);

        int x = 30;
        int y = 30;

        DrawText("X Controller:", x, y, 20, LIGHTGRAY);
        x += SPINNER_MARGIN;
        y -= 3;
        if (GuiValueBox(
                (Rectangle){x, y, SPINNER_W, SPINNER_H},
                "P: ",
                &x_p,
                0,
                MAX_INPUT,
                ui_state == X_P
            ))
            ui_state = X_P;
        x += SPINNER_W + SPINNER_SPACING;
        if (GuiValueBox(
                (Rectangle){x, y, SPINNER_W, SPINNER_H},
                "I: ",
                &x_i,
                0,
                MAX_INPUT,
                ui_state == X_I
            ))
            ui_state = X_I;
        x += SPINNER_W + SPINNER_SPACING;
        if (GuiValueBox(
                (Rectangle){x, y, SPINNER_W, SPINNER_H},
                "D: ",
                &x_d,
                0,
                MAX_INPUT,
                ui_state == X_D
            ))
            ui_state = X_D;

        DrawLine(
            10, 70, SPINNER_MARGIN + (SPINNER_W + SPINNER_SPACING) * 3, 70, GRAY
        );

        x = 30;
        y = 86;
        DrawText("Theta Controller:", x, y, 20, LIGHTGRAY);
        x += SPINNER_MARGIN;
        y -= 3;
        if (GuiValueBox(
                (Rectangle){x, y, SPINNER_W, SPINNER_H},
                "P: ",
                &theta_p,
                0,
                MAX_INPUT,
                ui_state == TH_P
            ))
            ui_state = TH_P;
        x += SPINNER_W + SPINNER_SPACING;
        if (GuiValueBox(
                (Rectangle){x, y, SPINNER_W, SPINNER_H},
                "I: ",
                &theta_i,
                0,
                MAX_INPUT,
                ui_state == TH_I
            ))
            ui_state = TH_I;
        x += SPINNER_W + SPINNER_SPACING;
        if (GuiValueBox(
                (Rectangle){x, y, SPINNER_W, SPINNER_H},
                "D: ",
                &theta_d,
                0,
                MAX_INPUT,
                ui_state == TH_D
            ))
            ui_state = TH_D;

        DrawText(
            TextFormat("FPS: %d", (int)(1.0 / GetFrameTime())),
            help_rect.x - 40,
            90,
            20,
            LIGHTGRAY
        );

        DrawRectangleRounded(cart, 0.2, 3, WHITE);
        DrawCircleV(pole_start, 6, LIGHTGRAY);
        DrawLineEx(pole_start, pole_end, 8, BEIGE);
        DrawLine(
            pole_start.x,
            pole_start.y,
            pole_start.x + forces.x / 64,
            pole_start.y,
            SKYBLUE
        );

        DrawRectangleRounded(help_rect, 0.1, 1, RED);
        DrawText("?", HELP_X, HELP_Y, HELP_FONT, LIGHTGRAY);
    }
    EndDrawing();
}

int main(void) {
    help_rect = (Rectangle){HELP_X - 13, HELP_Y - 8, 41, 41};

    // TODO stabilize for smaller angles
    sys = (System){.r = {WIN_W / 2, 3 * PI / (4.0)}, .v = {0, 0}, .a = {0, 0}};

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIN_W, WIN_H, "Cart Pole with PID controller");
    SetTargetFPS(FPS);

    // TODO : change focus and pressed color
    GuiSetStyle(0, TEXT_SIZE, 14);
    GuiSetStyle(0, TEXT_PADDING, 6);
    GuiSetStyle(0, BORDER_COLOR_NORMAL, 0xAAAAAAFF);
    GuiSetStyle(0, TEXT_COLOR_NORMAL, 0xDDDDDDFF);

    rayutl_mainloop(update_draw_frame, FPS);

    CloseWindow();
    return 0;
}