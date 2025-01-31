#ifndef MOTION_H
#define MOTION_H
#include <raymath.h>

#define MOT_MIN_STEP 8e-3
#define MOT_MAX_STEP 3e-2
#define MOT_MIN_ERR 1e-3
#define MOT_MAX_ERR 1e-1

typedef struct {
    bool is_static;
    Vector2 r;
    Vector2 v;
    Vector2 a;
    float mass;
} Particle;

void mot_integrate_backward_euler(Vector2 *r, Vector2 *v, Vector2 a, float dt);
void mot_integrate_symplectic_euler(
    Vector2 *r, Vector2 *v, Vector2 prev_a, float dt
);
void mot_integrate_verlet(
    Vector2 *r, Vector2 a, Vector2 prev_r, float dt, float drag
);
// Velocity Verlet integration
void mot_integrate_v_verlet(
    Vector2 *r, Vector2 *v, Vector2 a, Vector2 prev_a, float dt
);
// Adaptive sized velocity Verlet integration
void mot_adaptive_verlet(
    Vector2 *r, Vector2 *v, Vector2 a, Vector2 prev_a, float dt
);

// 4th order Runge-Kutta integration
void mot_integrate_rk4(
    Vector2 *r, Vector2 *v, Vector2 a, Vector2 prev_a, float dt
);
// Adaptive sized velocity 4th order Runge-Kutta integration
void mot_adaptive_rk4(
    Vector2 *r, Vector2 *v, Vector2 a, Vector2 prev_a, float dt
);
#ifdef MOTION_IMPLEMENTATION

inline void mot_integrate_backward_euler(
    Vector2 *r, Vector2 *v, Vector2 next_a, float dt
) {
    *v = Vector2Add(*v, Vector2Scale(next_a, dt));
    *r = Vector2Add(*r, Vector2Scale(*v, dt));
}

inline void mot_integrate_symplectic_euler(
    Vector2 *r, Vector2 *v, Vector2 curr_a, float dt
) {
    *v = Vector2Add(*v, Vector2Scale(curr_a, dt));
    *r = Vector2Add(*r, Vector2Scale(*v, dt));
}

inline void mot_integrate_verlet(
    Vector2 *curr_r, Vector2 a, Vector2 prev_r, float dt, float drag
) {
    *curr_r = Vector2Add(
        Vector2Subtract(
            Vector2Scale(*curr_r, 2.0 - drag), Vector2Scale(prev_r, 1.0 - drag)
        ),
        Vector2Scale(a, dt * dt)
    );
}

void mot_integrate_v_verlet(
    Vector2 *r, Vector2 *v, Vector2 a, Vector2 prev_a, float dt
) {
    Vector2 half_v = Vector2Add(*v, Vector2Scale(prev_a, dt / 2));
    *r = Vector2Add(*r, Vector2Scale(half_v, dt));
    *v = Vector2Add(half_v, Vector2Scale(a, dt / 2));
}

void mot_adaptive_verlet(
    Vector2 *r, Vector2 *v, Vector2 a, Vector2 prev_a, float dt
) {
    float h = dt;
    float spent_time = 0;
    while (spent_time < 0.9 * dt) {
        Vector2 normal_r = *r;
        Vector2 normal_v = *v;
        mot_integrate_v_verlet(&normal_r, &normal_v, a, prev_a, h);

        Vector2 half_r = *r;
        Vector2 half_v = *v;
        mot_integrate_v_verlet(&half_r, &half_v, a, prev_a, h / 2);

        if (h > MOT_MIN_STEP &&
            Vector2Distance(normal_r, half_r) > MOT_MAX_ERR) {
            h /= 2;
            continue;
        }

        Vector2 double_r = *r;
        Vector2 double_v = *v;
        mot_integrate_v_verlet(&double_r, &double_v, a, prev_a, h * 2);

        if (h < MOT_MAX_STEP &&
            Vector2Distance(normal_r, double_r) < MOT_MIN_ERR) {
            h *= 2;
            continue;
        }

        *r = normal_r;
        *v = normal_v;
        spent_time += h;
    }
}

void mot_integrate_rk4(
    Vector2 *r, Vector2 *v, Vector2 a, Vector2 prev_a, float dt
) {
    Vector2 r_array[4];
    Vector2 v_array[4];
    Vector2 a_array[4];

    r_array[0] = *r;
    v_array[0] = *v;
    a_array[0] = prev_a;

    r_array[1] = Vector2Add(r_array[0], Vector2Scale(v_array[0], 0.5 * dt));
    v_array[1] = Vector2Add(v_array[0], Vector2Scale(a_array[0], 0.5 * dt));
    a_array[1] = Vector2Scale(Vector2Add(prev_a, a), 0.5);

    r_array[2] = Vector2Add(r_array[1], Vector2Scale(v_array[1], 0.5 * dt));
    v_array[2] = Vector2Add(v_array[1], Vector2Scale(a_array[1], 0.5 * dt));
    a_array[2] = a_array[1];

    r_array[3] = Vector2Add(r_array[2], Vector2Scale(v_array[2], dt));
    v_array[3] = Vector2Add(v_array[2], Vector2Scale(a_array[2], dt));
    a_array[3] = a;

    // Going for:
    // x1 = x0 + dt/h * (v1 + 2 * v2 + 2 * v3 + v4)
    // v1 = v0 + dt/h * (a1 + 2 * a2 + 2 * a3 + a4)

    // Calculate the part inside the last parentheses
    Vector2 v_sum = Vector2Zero();
    Vector2 a_sum = Vector2Zero();

    v_array[1] = Vector2Scale(v_array[1], 2);
    v_array[2] = Vector2Scale(v_array[2], 2);
    a_array[1] = Vector2Scale(a_array[1], 2);
    a_array[2] = Vector2Scale(a_array[2], 2);

    for (size_t i = 0; i < 4; i++) {
        v_sum = Vector2Add(v_sum, v_array[i]);
        a_sum = Vector2Add(a_sum, a_array[i]);
    }

    *r = Vector2Add(*r, Vector2Scale(v_sum, dt / 6));
    *v = Vector2Add(*v, Vector2Scale(a_sum, dt / 6));
}

void mot_adaptive_rk4(
    Vector2 *r, Vector2 *v, Vector2 a, Vector2 prev_a, float dt
) {
    float h = dt;
    float spent_time = 0;
    while (spent_time < 0.9 * dt) {
        Vector2 normal_r = *r;
        Vector2 normal_v = *v;
        mot_integrate_rk4(&normal_r, &normal_v, a, prev_a, h);

        Vector2 half_r = *r;
        Vector2 half_v = *v;
        mot_integrate_rk4(&half_r, &half_v, a, prev_a, h / 2);

        if (h > MOT_MIN_STEP &&
            Vector2Distance(normal_r, half_r) > MOT_MAX_ERR) {
            h /= 2;
            continue;
        }

        Vector2 double_r = *r;
        Vector2 double_v = *v;
        mot_integrate_rk4(&double_r, &double_v, a, prev_a, h * 2);

        if (h < MOT_MAX_STEP &&
            Vector2Distance(normal_r, double_r) < MOT_MIN_ERR) {
            h *= 2;
            continue;
        }

        *r = normal_r;
        *v = normal_v;
        spent_time += h;
    }
}

#endif  // end of MOTION_IMPLEMENTATION
#endif  // end of header guard