// 2D cloth simulation
// based on
// http://web.archive.org/web/20070610223835/http://www.teknikus.dk/tj/gdc2001.htm

#include <stdlib.h>
#include <math.h>
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
#define SPEED 0.9
#define G 1e5  // it doesn't have to be realistic
#define TRACE_SIZE 222

typedef struct {
    Vector2 *items;
    size_t capacity;
    size_t count;
} Vector2Buffer;

typedef struct {
    Particle *items;
    size_t capacity;
    size_t count;
} Particles;

// An array with rotating index
typedef struct {
    Vector2 points[TRACE_SIZE];
    size_t index;
} Trace;

typedef struct {
    Trace *items;
    size_t capacity;
    size_t count;
} Traces;

/* Declarations */
Particles particles;
Vector2Buffer a_buffer;
Traces traces;

int traced_frames = 0;
bool paused = false;
bool dragging = false;
Particle *clicked_node = NULL;

const int HELP_X = WIN_W - 50;
const int HELP_Y = 36;
const int HELP_FONT = 27;
Rectangle help_rect;
bool show_help = false;

// TODO : change
const char *help_text =
    "You can use your cursor to drag particles. \n\n\n\n"
    "Also by clicking on particles you can lock/unlock them. \n\n\n\n"
    // TODO : implement:
    //"If you press right click on a point it will be deleted.\n\n\n\n"
    "Mouse wheen / left and right arrow: adjust wind speed \n\n\n\n"
    "Plus / Minus: adjust gravity \n\n\n\n"
    "Space: pause the simulation \n\n\n\n"
    "Enter: advance simulation one step by pressing enter. \n\n\n\n"
    " \n\n\n\n"
    "Press Enter or click anywhere to resume. \n\n\n\n";

/* End of declarations */

// Does a linear search though all particles.
// Only is performant for not too large numbers of particles
// Returns null if there's no particle in a radius of distance/2
Particle *nearest_particle(Particles particles, Vector2 pos) {
    float least_distance = WIN_W / 500;
    Particle *result = NULL;
    for (size_t i = 0; i < particles.count; i++) {
        float distance =
            Vector2Length(Vector2Subtract(particles.items[i].r, pos));
        if (distance <= least_distance) {
            least_distance = distance;
            result = particles.items + i;
        }
    }
    return result;
}

float particle_radius(float mass) { return mass / 5; }

void update_particle(Particle *p, Vector2 prev_a, float dt) {
    mot_adaptive_verlet(&p->r, &p->v, p->a, prev_a, dt);

    // Bounce particle if it's going out of screen
    if (p->r.x < 0) {
        p->v.x = fabs(p->v.x);
    } else if (p->r.x > WIN_W) {
        p->v.x = -fabs(p->v.x);
    }

    if (p->r.y < 0) {
        p->v.y = fabs(p->v.y);
    } else if (p->r.y > WIN_H) {
        p->v.y = -fabs(p->v.y);
    }
}

void update_physics(float dt) {
    // Accumulate forces for each particle and check collisions
    for (size_t i = 0; i < particles.count; i++) {
        for (size_t j = i + 1; j < particles.count; j++) {
            Particle *p1 = particles.items + i;
            Particle *p2 = particles.items + j;
            float p1_radius = particle_radius(p1->mass);
            float p2_radius = particle_radius(p2->mass);
            float ideal_distance = p1_radius + p2_radius;

            float distance = Vector2Distance(p1->r, p2->r);
            float distance_sq = powf(distance, 2.0);

            Vector2 unit_displacement =
                Vector2Normalize(Vector2Subtract(p2->r, p1->r));

            p1->a = Vector2Scale(unit_displacement, G * p2->mass / distance_sq);
            p2->a =
                Vector2Scale(unit_displacement, -G * p1->mass / distance_sq);

            // Check particles for collisions and set responses
            if (distance <= ideal_distance) {
                float p = 2 *
                          (Vector2DotProduct(p1->v, unit_displacement) -
                           Vector2DotProduct(p2->v, unit_displacement)) /
                          (p1->mass + p2->mass);
                p1->v = Vector2Subtract(
                    p1->v, Vector2Scale(unit_displacement, p * p1->mass)
                );
                p2->v = Vector2Add(
                    p2->v, Vector2Scale(unit_displacement, p * p2->mass)
                );
            }
        }
    }

    // Change particles' position based on their acceleration
    for (size_t i = 0; i < particles.count; i++) {
        Particle *p = particles.items + i;
        update_particle(p, a_buffer.items[i], dt);
        a_buffer.items[i] = p->a;
        // Update particle trace
        Trace *trace = traces.items + i;
        trace->index = utl_safe_wrap(trace->index - 1, TRACE_SIZE);
        trace->points[trace->index] = p->r;
    }
}

void update_draw_frame(void) {
    // Handle input
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;
    if (IsKeyPressed(KEY_ENTER)) update_physics(1 / (float)FPS);

    Vector2 mouse_pos = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        // Set clicked_particle before dragging starts
        // so it doesn't change with mouse pos
        if (!dragging) {
            clicked_node = nearest_particle(particles, mouse_pos);
        }
        // Initiate dragging if mouse position delta is big enough
        if (Vector2Length(GetMouseDelta()) > 1.0 && clicked_node != NULL) {
            dragging = true;
        }
        // Update particle position based on mouse position
        if (dragging) {
            Vector2 drag_acc = Vector2Scale(
                Vector2Subtract(mouse_pos, clicked_node->r), 10.0
            );
            mot_integrate_verlet(
                &clicked_node->r, drag_acc, clicked_node->r, 0.1, 0
            );
        }
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (dragging) {
            dragging = false;
        } else if (clicked_node != NULL) {
            clicked_node->is_static = !clicked_node->is_static;
        } else if (CheckCollisionPointRec(mouse_pos, help_rect)) {
            show_help = true;
        }
    }

    // Show help dialog
    if (show_help) {
        BeginDrawing();
        DrawRectangle(100, 50, WIN_H - 200, WIN_W - 100, LIGHTGRAY);
        DrawText(help_text, 170, 130, 24, BLACK);
        EndDrawing();

        if (IsKeyReleased(KEY_ENTER) ||
            IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            show_help = false;
        }
        return;
    }

    if (!paused) update_physics(1 / (float)FPS);

    int trace_count = TRACE_SIZE;
    if (traced_frames < trace_count) {
        trace_count = traced_frames;
        traced_frames++;
    }

    BeginDrawing();
    {
        ClearBackground(BLACK);
        DrawText(
            TextFormat("FPS: %d", (int)(1.0 / GetFrameTime())), 50, 50, 20,
            WHITE
        );

        for (size_t i = 0; i < particles.count; i++) {
            const Particle p = particles.items[i];
            DrawCircle(
                p.r.x, p.r.y, particle_radius(p.mass), p.is_static ? RED : WHITE
            );

            // Draw particle's trace
            Trace trace = traces.items[i];
            float alpha = 1.0;
            for (int i = 0; i < trace_count - 1; i++) {
                size_t index = utl_safe_wrap(i + trace.index, TRACE_SIZE);
                size_t next_index =
                    utl_safe_wrap(1 + i + trace.index, TRACE_SIZE);
                DrawLineV(
                    trace.points[index], trace.points[next_index],
                    ColorAlpha(WHITE, alpha)
                );
                alpha *= 0.987;  // fadeout trace
            }
        }

        DrawRectangleRounded(help_rect, 0.1, 1, RED);
        DrawText("?", HELP_X, HELP_Y, HELP_FONT, LIGHTGRAY);
    }
    EndDrawing();
}

int main(void) {
    help_rect = (Rectangle){HELP_X - 13, HELP_Y - 8, 41, 41};
    utl_da_init(particles, 0);
    utl_da_init(a_buffer, particles.capacity);
    a_buffer.count = a_buffer.capacity;

    if (particles.items == NULL || a_buffer.items == NULL) {
        utl_log(UTL_ERROR, "Couldn't allocate memory for simulation.");
        exit(-1);
    }

    // Initialize particles
    memset(a_buffer.items, 0, a_buffer.capacity * sizeof(*a_buffer.items));

    Particle test[] = {
        {
            .r = (Vector2){WIN_W / 2, WIN_H / 2},
            .v = (Vector2){0, 240},
            .a = Vector2Zero(),
            .mass = 120,
            .is_static = false,
        },
        {
            .r = (Vector2){WIN_W / 2 - 200, WIN_H / 2},
            .v = (Vector2){0, -240},
            .a = Vector2Zero(),
            .mass = 120,
            .is_static = false,
        },
        {
            .r = (Vector2){WIN_W / 2 + 200, WIN_H / 2},
            .v = (Vector2){0, 0},
            .a = Vector2Zero(),
            .mass = 81,
            .is_static = false,
        }
    };

    utl_da_append_many(particles, test, utl_array_size(test));

    // Initialize traces to out of screen
    utl_da_init(traces, 0);
    for (size_t i = 0; i < particles.count; i++) {
        utl_da_append(traces, (Trace){0});
    }

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIN_W, WIN_H, "N-Body Simulation");
    SetTargetFPS(FPS);

    rayutl_mainloop(update_draw_frame, FPS);

    CloseWindow();
    utl_da_free(a_buffer);
    utl_da_free(particles);

    return 0;
}