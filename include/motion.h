#ifndef MOTION_H
#define MOTION_H
#include <raymath.h>

typedef struct {
  bool is_static;
  Vector2 r;
  Vector2 v;
  Vector2 a;
  float mass;
} Particle;

void mot_integrate_backward_euler(Vector2 *r, Vector2 *v, Vector2 a, float dt);
void mot_integrate_symplectic_euler(Vector2 *r, Vector2 *v, Vector2 prev_a,
                                    float dt);
void mot_integrate_verlet(Vector2 *r, Vector2 a, Vector2 prev_r, float dt,
                          float drag);

#ifdef MOTION_IMPLEMENTATION

inline void mot_integrate_backward_euler(Vector2 *r, Vector2 *v, Vector2 next_a,
                                         float dt) {
  *v = Vector2Add(*v, Vector2Scale(next_a, dt));
  *r = Vector2Add(*r, Vector2Scale(*v, dt));
}

inline void mot_integrate_symplectic_euler(Vector2 *r, Vector2 *v,
                                           Vector2 curr_a, float dt) {
  *v = Vector2Add(*v, Vector2Scale(curr_a, dt));
  *r = Vector2Add(*r, Vector2Scale(*v, dt));
}

inline void mot_integrate_verlet(Vector2 *curr_r, Vector2 a, Vector2 prev_r,
                                 float dt, float drag) {
  *curr_r = Vector2Add(Vector2Subtract(Vector2Scale(*curr_r, 2.0 - drag),
                                       Vector2Scale(prev_r, 1.0 - drag)),
                       Vector2Scale(a, dt * dt));
}

#endif // end of MOTION_IMPLEMENTATION
#endif // end of header guard