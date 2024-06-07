// 2D cloth simulation
// based on
// http://web.archive.org/web/20070610223835/http://www.teknikus.dk/tj/gdc2001.htm

#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raymath.h"
#define MOTION_IMPLEMENTATION
#include "motion.h"
#define UTL_IMPLEMENTATION
#include "rayutl.h"
#include "utl.h"

#define FPS 100
#define SCREEN_W 700
#define SCREEN_H 1000
#define GRID_W 21

#define GRID_H 13
#define DISTANCE 27.0
#define RADIUS 2
#define PARTICLE_MASS 10.0
#define INIT_G 98.1
#define DRAG 0.01
#define SPEED 5.0

typedef struct {
    Vector2 *items;
    size_t capacity;
    size_t count;
} Vector2Buffer;

typedef struct {
    Particle *items;
    size_t capacity;
    size_t count;
} Points;

typedef struct {
    Particle *p1;
    Particle *p2;
    int size;
} Link;

typedef struct {
    Link *items;
    size_t capacity;
    size_t count;
} Links;

#define index2d(pointer, x, y) ((pointer) + ((y) * GRID_W + (x)))

/* Declarations */
Points points;
Links links;
Vector2Buffer buffer;

bool paused = false;
bool dragging = false;
Particle *clicked_particle = NULL;

Vector2 g = {0, INIT_G};
Vector2 wind = {0, 0};

const float start_x = (SCREEN_H - (GRID_W - 1) * DISTANCE) / 2;
const float start_y = (SCREEN_W - (GRID_H - 1) * DISTANCE) / 4;

const int help_x = SCREEN_H - 50;
const int help_y = 36;
const int help_font = 27;
Rectangle help_rect;
bool show_help = false;

const char *help_text =
    "You can use your cursor to drag points. \n\n\n\n"
    "Also by clicking on points you can lock/unlock them. \n\n\n\n"
    // TODO : implement:
    //"If you press right click on a point it will be deleted.\n\n\n\n"
    "Mouse wheen / left and right arrow: adjust wind speed \n\n\n\n"
    "Plus / Minus: adjust gravity \n\n\n\n"
    "Space: pause the simulation \n\n\n\n"
    "Enter: advance simulation one step by pressing enter. \n\n\n\n"
    " \n\n\n\n"
    "Press Enter or click anywhere to resume. \n\n\n\n";
/* End of declarations */


void update_physics(float dt) {
    // Accumulate forces for each particle
    for (size_t i = 0; i < points.count; i++) {
        Particle *p = points.items + i;

        Vector2 force = Vector2Zero();
        // add gravity's force
        force = Vector2Add(force, Vector2Scale(g, p->mass));
        // add wind's effect to the force
        force = Vector2Add(force, wind);
        // Set acceleration to force/mass
        p->a = Vector2Scale(force, 1 / p->mass);
    }

    // Change particles' position based on their acceleration
    for (size_t i = 0; i < points.count; i++) {
        Particle *p = points.items + i;
        if (p->is_static) continue;

        // Apply verlet integration and store old position into buffer[i]
        Vector2 temp = p->r;
        mot_integrate_verlet(&p->r, p->a, buffer.items[i], SPEED * dt, DRAG);
        buffer.items[i] = temp;
    }

    // Apply link constraints
    for (size_t i = 0; i < links.count; i++) {
        Particle *p1 = links.items[i].p1;
        Particle *p2 = links.items[i].p2;
        Vector2 delta = Vector2Subtract(p2->r, p1->r);
        float delta_len = Vector2Length(delta);
        // displacement = delta * 0.5 * diff, and
        // diff = (delta_len - link.size)/delta_len
        Vector2 displacement = Vector2Scale(
            delta, 0.5 * (delta_len - links.items[i].size) / delta_len);
        if (!p1->is_static) p1->r = Vector2Add(p1->r, displacement);
        if (!p2->is_static) p2->r = Vector2Subtract(p2->r, displacement);
    }
}

// Does a linear search though all points.
// Only is performant for not too large numbers of points
// Returns null if there's no particle in a radius of distance/2
Particle *nearest_particle(Points points, Vector2 pos) {
    float least_distance = DISTANCE / 2;
    Particle *result = NULL;
    for (size_t i = 0; i < points.count; i++) {
        float distance = Vector2Length(Vector2Subtract(points.items[i].r, pos));
        if (distance <= least_distance) {
            least_distance = distance;
            result = points.items + i;
        }
    }
    return result;
}

void UpdateDrawFrame(void) {
    // Handle input
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;
    if (IsKeyPressed(KEY_ENTER)) update_physics(1 / (float)FPS);
    if (IsKeyPressed(KEY_LEFT)) wind.x -= 100.0;
    if (IsKeyPressed(KEY_RIGHT)) wind.x += 100.0;
    if (IsKeyPressed(KEY_MINUS)) g.y -= INIT_G / 2;
    if (IsKeyPressed(KEY_EQUAL)) g.y += INIT_G / 2;

    wind.x += GetMouseWheelMove() * 40.0;
    Vector2 mouse_pos = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        // Set clicked_particle before dragging starts
        // so it doesn't change with mouse pos
        if (!dragging) {
            clicked_particle = nearest_particle(points, mouse_pos);
        }
        // Initiate dragging if mouse position delta is big enough
        if (Vector2Length(GetMouseDelta()) > 1.0 && clicked_particle != NULL) {
            dragging = true;
        }
        // Update particle position based on mouse position
        if (dragging) {
            Vector2 drag_acc = Vector2Scale(
                Vector2Subtract(mouse_pos, clicked_particle->r), 10.0);
            mot_integrate_verlet(&clicked_particle->r, drag_acc,
                                 clicked_particle->r, 0.1, 0);
        }
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (dragging) {
            dragging = false;
        } else if (clicked_particle != NULL) {
            clicked_particle->is_static = !clicked_particle->is_static;
        } else if (CheckCollisionPointRec(mouse_pos, help_rect)) {
            show_help = true;
        }
    }

    // Show help dialog
    if (show_help) {
        BeginDrawing();
        DrawRectangle(100, 50, SCREEN_H - 200, SCREEN_W - 100, LIGHTGRAY);
        // TODO : change font family
        DrawText(help_text, 170, 130, 24, BLACK);
        EndDrawing();

        if (IsKeyReleased(KEY_ENTER) ||
            IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            show_help = false;
        }
        return;
    }

    if (!paused) update_physics(1 / (float)FPS);

    BeginDrawing();
    {
        ClearBackground(BLACK);

        for (size_t i = 0; i < links.count; i++) {
            Link link = links.items[i];
            DrawLine(link.p1->r.x, link.p1->r.y, link.p2->r.x, link.p2->r.y,
                     GRAY);
        }
        for (size_t i = 0; i < points.count; i++) {
            const Particle p = points.items[i];
            DrawCircle(p.r.x, p.r.y, RADIUS, p.is_static ? RED : WHITE);
        }

        DrawRectangleRounded(help_rect, 0.1, 1, RED);
        DrawText("?", help_x, help_y, help_font, LIGHTGRAY);
    }
    EndDrawing();
}

int main(void) {
    help_rect = (Rectangle){help_x - 13, help_y - 8, 41, 41};
    utl_da_init(points, GRID_W * GRID_H);
    utl_da_init(links, 0);
    utl_da_init(buffer, points.capacity);
    buffer.count = buffer.capacity;

    if (points.items == NULL || links.items == NULL || buffer.items == NULL) {
        utl_log(UTL_ERROR, "Couldn't allocate memory for simulation.");
        exit(-1);
    }

    // Initialize points
    memset(points.items, 0, points.capacity * sizeof(*points.items));
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            Particle *p = index2d(points.items, x, y);
            p->r.x = start_x + x * DISTANCE;
            p->r.y = start_y + y * DISTANCE;
            p->mass = PARTICLE_MASS;
        }
    }
    points.count = GRID_W * GRID_H;

    // Set up static points
    (points.items + 0)->is_static = true;
    (points.items + GRID_W / 2)->is_static = true;
    (points.items + GRID_W - 1)->is_static = true;

    // Initialize buffer with starting positions
    for (size_t i = 0; i < points.count; i++) {
        buffer.items[i] = points.items[i].r;
    }

    // Initialize links
    // const float diagonal = DISTANCE * 1.41421356237; // distance*sqrt(2)
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            // Connect to next horizontal particle
            if (x != GRID_W - 1) {
                Link link = {index2d(points.items, x, y),
                             index2d(points.items, x + 1, y), DISTANCE};
                utl_da_append(links, link);
            }
            // Connect to next vertical particle
            if (y != GRID_H - 1) {
                Link link = {index2d(points.items, x, y),
                             index2d(points.items, x, y + 1), DISTANCE};
                utl_da_append(links, link);
            }
            // // Connect to next diagonal particle
            // if ((x != GRID_W - 1) && (y != GRID_H - 1)) {
            //   Link link = {index2d(points.items, x, y),
            //                index2d(points.items, x + 1, y + 1), diagonal};
            //   utl_da_append(&links, link);
            // }
            // // Connect to previous diagonal particle
            // if ((x != 0) && (y != GRID_H - 1)) {
            //   Link link = {index2d(points.items, x, y),
            //                index2d(points.items, x - 1, y + 1), diagonal};
            //   utl_da_append(&links, link);
            // }
        }
    }
    utl_log(UTL_DEBUG, "links count: %d, cap: %d", links.count, links.capacity);

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_H, SCREEN_W, "2D Cloth Simulation");
    SetTargetFPS(FPS);

    rayutl_mainloop(UpdateDrawFrame, FPS);

    CloseWindow();
    utl_da_free(links);
    utl_da_free(buffer);
    utl_da_free(points);

    return 0;
}