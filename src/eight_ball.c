// 8 Ball pool

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#define MOTION_IMPLEMENTATION
#include "motion.h"
#define UTL_IMPLEMENTATION
#include "rayutl.h"
#include "utl.h"

// TODO : cue stick
// TODO : draw diamonds
// TODO : simulate ball spin
// TODO : coefficient of restitution

//// UI related constants
#define WIN_W 1600
#define WIN_H (WIN_W / 2)
#define TABLE_W 22
// Correct for current config, as it should be around 0.012 of TABLE_W
// BALL_R = (WIN_W*0.012*(1-2*TABLE_MARGIN));
// I'm consistently using ball radius as a unit of measure throughout the code
#define BALL_R 16
#define POCKET_R (BALL_R * 2.1)
#define TABLE_MARGIN 0.1
#define HEADSTRING_X (WIN_W * (1 + 2 * TABLE_MARGIN)) / 4
#define HINT_LEN_MIN 2
#define HINT_LEN_MAX 6

//// Physics related constants
#define CLOTH_DRAG 0.72
#define VELOCITY_SQR_MIN 10
#define POCKET_ZONE_MIN_DISTANCE (POCKET_R / 4)
#define POCKET_ZONE_MAX_DISTANCE (POCKET_R * 3 / 4)
#define POCKET_MAX_VELOCITY 1000
#define POCKET_MIN_VELOCITY 20
#define HIT_POWER_MIN 40
#define HIT_POWER_MAX 3600
#define HIT_DISTANCE_MAX 300

//// Misc. game constants
#define FPS 0
#define BALL_COUNT 15

///// COLORS
#define BG_COLOR ((Color){42, 41, 40, 255})
#define TABLE_COLOR ((Color){100, 60, 40, 255})
#define CLOTH_COLOR ((Color){35, 120, 72, 255})
// Ball colors
#define B_YELLOW {253, 210, 0, 255}
#define B_BLUE {30, 90, 220, 255}
#define B_RED {230, 55, 55, 255}
#define B_PURPLE {100, 31, 110, 255}
#define B_ORANGE {255, 120, 30, 255}
#define B_GREEN {0, 180, 120, 255}
#define B_BROWN {120, 70, 45, 255}
#define B_BLACK {0, 0, 0, 255}

typedef enum {
    STATE_STARTING,
    STATE_HITTING,
    STATE_MOTION,
    STATE_BALL_IN_HAND,
} State;

typedef enum {
    SOLID_BALL,
    STRIPED_BALL,
} BallType;

typedef struct {
    int number;
    BallType type;
    Color color;
    Vector2 pos;
    Vector2 vel;
    Vector2 prev_acc;
    bool is_pocketed;
} Ball;

const Rectangle CLOTH_RECT = {
    .x = WIN_W * TABLE_MARGIN,
    .y = WIN_H * TABLE_MARGIN - TABLE_W,  // move the table a bit to the top
    .width = WIN_W * (1 - 2 * TABLE_MARGIN),
    .height = WIN_H * (1 - 2 * TABLE_MARGIN),
};

const Rectangle TABLE_RECT = {
    .x = WIN_W * TABLE_MARGIN - TABLE_W,
    .y = WIN_H * TABLE_MARGIN - TABLE_W * 2,
    .width = WIN_W * (1 - 2 * TABLE_MARGIN) + 2 * TABLE_W,
    .height = WIN_H * (1 - 2 * TABLE_MARGIN) + 2 * TABLE_W,
};

// Ball in hand request button rectangle
const Rectangle BIH_REQ_RECT = {
    .x = 0.2 * WIN_W,
    .y = 0.925 * WIN_H,
    .width = 0.6 * WIN_W,
    .height = 0.05 * WIN_H
};

int pocket_ys[2];
int pocket_xs[3];
int center_pocket_check_ys[2];
Vector2 hit_mouse_pos;
State game_state = STATE_STARTING;
float hit_power = HIT_POWER_MAX / 2;
Ball balls[16] = {
    {
        .number = 1,
        .type = SOLID_BALL,
        .color = B_YELLOW,
    },
    {
        .number = 2,
        .type = SOLID_BALL,
        .color = B_BLUE,
    },
    {
        .number = 3,
        .type = SOLID_BALL,
        .color = B_RED,
    },
    {
        .number = 4,
        .type = SOLID_BALL,
        .color = B_PURPLE,
    },
    {
        // 8 ball's 5th in the rack(counting from top left)
        // and it's position is fixed
        .number = 8,
        .type = SOLID_BALL,
        .color = B_BLACK,
    },
    {
        .number = 5,
        .type = SOLID_BALL,
        .color = B_ORANGE,
    },
    {
        .number = 6,
        .type = SOLID_BALL,
        .color = B_GREEN,
    },
    {
        .number = 7,
        .type = SOLID_BALL,
        .color = B_BROWN,
    },
    {
        .number = 9,
        .type = STRIPED_BALL,
        .color = B_YELLOW,
    },
    {
        .number = 10,
        .type = STRIPED_BALL,
        .color = B_BLUE,
    },
    {
        .number = 11,
        .type = STRIPED_BALL,
        .color = B_RED,
    },
    {
        .number = 12,
        .type = STRIPED_BALL,
        .color = B_PURPLE,
    },
    {
        .number = 13,
        .type = STRIPED_BALL,
        .color = B_ORANGE,
    },
    {
        .number = 14,
        .type = STRIPED_BALL,
        .color = B_GREEN,
    },
    {
        .number = 15,
        .type = STRIPED_BALL,
        .color = B_BROWN,
    },
    {// Storing the cue ball as the last element of array to make some
     //  calculations easier to write
     .number = 0,
     .vel = {0, 0}
    },
};

void shuffle_balls() {
    for (size_t i = 0; i < BALL_COUNT - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (BALL_COUNT - i) + 1);
        if (i == 4 || j == 4) continue;  // don't move 8th ball
        Ball temp = balls[j];
        balls[j] = balls[i];
        balls[i] = temp;
    }
    // Don't let the balls in top right and bottom right be of the same suit
    while (balls[10].type == balls[14].type) {
        size_t i = rand() / (RAND_MAX / (BALL_COUNT - 1) + 1);
        Ball temp = balls[10];
        balls[10] = balls[i];
        balls[i] = temp;
    }
}

void init_balls() {
    const float start_x = (CLOTH_RECT.x + CLOTH_RECT.width) * 3 / 4;
    const float start_y = CLOTH_RECT.y + CLOTH_RECT.height / 2;
    const float spacing_x = BALL_R * 13 / 6;
    const float spacing_y = BALL_R * 5 / 2;

    shuffle_balls();

    size_t i = 0;
    for (size_t col = 0; col < 5; col++) {
        for (size_t y = 0; y <= col; y++, i++) {
            balls[i].pos.x = start_x + col * spacing_x;
            balls[i].pos.y =
                start_y + ((float)y - (float)col / 2.0) * spacing_y;
        }
    }
}

void draw_ellipse_for_pocket(float x, float y, float rotation, bool is_center) {
    rlPushMatrix();
    rlTranslatef(x, y, 0);
    rlRotatef(rotation, 0, 0, 1);
    DrawEllipse(
        0, 0, POCKET_R - 1, is_center ? POCKET_R * 3 / 5 : POCKET_R / 2, BLACK
    );
    rlPopMatrix();
}

void draw_background() {
    ClearBackground(BG_COLOR);
    DrawRectangleRounded(TABLE_RECT, 0.1, 50, TABLE_COLOR);
    DrawRectangleRec(CLOTH_RECT, CLOTH_COLOR);

    // Draw holes then hide them with the cloth and draw a projection of them
    // using ellipses
    for (size_t x = 0; x < 3; x++) {
        for (size_t y = 0; y < 2; y++) {
            DrawCircle(pocket_xs[x], pocket_ys[y], POCKET_R, BLACK);
        }
    }
    DrawRectangleRounded(CLOTH_RECT, 0.15, 50, CLOTH_COLOR);

    DrawLine( // Draw Headstring
        HEADSTRING_X,
        CLOTH_RECT.y,
        HEADSTRING_X,
        CLOTH_RECT.y + CLOTH_RECT.height,
        LIGHTGRAY
    );

    draw_ellipse_for_pocket(
        pocket_xs[0] + POCKET_R / 5, pocket_ys[0] + POCKET_R / 5, 315, false
    );
    draw_ellipse_for_pocket(
        pocket_xs[0] + POCKET_R / 5, pocket_ys[1] - POCKET_R / 5, 45, false
    );
    draw_ellipse_for_pocket(
        pocket_xs[1], pocket_ys[0] - POCKET_R / 2.5, 0, true
    );
    draw_ellipse_for_pocket(
        pocket_xs[1], pocket_ys[1] + POCKET_R / 2.5, 0, true
    );
    draw_ellipse_for_pocket(
        pocket_xs[2] - POCKET_R / 5, pocket_ys[0] + POCKET_R / 5, 45, false
    );
    draw_ellipse_for_pocket(
        pocket_xs[2] - POCKET_R / 5, pocket_ys[1] - POCKET_R / 5, 315, false
    );

#ifdef DEBUG
    for (size_t x = 0; x < 3; x++) {
        for (size_t y = 0; y < 2; y++) {
            Vector2 pos;
            if (x == 1) {
                pos = (Vector2){pocket_xs[x], center_pocket_check_ys[y]};
            } else {
                pos = (Vector2){pocket_xs[x], pocket_ys[y]};
            }
            DrawCircleLinesV(pos, POCKET_ZONE_MIN_DISTANCE, PURPLE);
            DrawCircleLinesV(pos, POCKET_ZONE_MAX_DISTANCE, PURPLE);
        }
    }
#endif
}

void draw_ball(Ball b, Vector2 pos) {
    DrawCircleV(pos, BALL_R, WHITE);

    if (b.type == SOLID_BALL) {
        DrawRing(pos, BALL_R - 7, BALL_R, 0, 360, BALL_R * 2.7, b.color);
    } else {
        DrawRectangle(pos.x - BALL_R, pos.y - 5, BALL_R * 2, 10, b.color);
        DrawCircleV(pos, BALL_R - 8, WHITE);  // (the stripe)
    }

    const int x_offset = b.number < 10 ? BALL_R * 2 / 7 : BALL_R * 3 / 7;
    DrawText(
        TextFormat("%d", b.number),
        pos.x - x_offset,
        pos.y - BALL_R / 2,
        BALL_R,
        BLACK
    );
}

void render() {
    const Vector2 mouse_pos = GetMousePosition();
    const Ball cue_ball = balls[BALL_COUNT];
    const Vector2 cue_dir =
        Vector2Normalize(Vector2Subtract(mouse_pos, cue_ball.pos));
    size_t pocketed_idx = 0;

    BeginDrawing();

    draw_background();

    // Draw balls
    const float start_x = WIN_W / 40;
    const float start_y = WIN_H / 8;
    for (size_t i = 0; i < BALL_COUNT; i++) {
        Ball ball = balls[i];
        Vector2 pos;
        if (ball.is_pocketed) {
            pos.x = 40 + pocketed_idx % 2 * BALL_R * 3;
            pos.y = 100 + pocketed_idx / 2 * BALL_R * 3;
            pocketed_idx++;
        } else {
            pos = ball.pos;
        }
        draw_ball(ball, pos);
    }
    if (pocketed_idx > 0)
        DrawText("Pocketed\nballs:", start_x / 2, start_y - 64, 17, RAYWHITE);

    if (!cue_ball.is_pocketed) DrawCircleV(cue_ball.pos, BALL_R, WHITE);

    // Draw "Request ball in hand" button
    const int btn_font_size = 20;
    const char *btn_text = "Request ball in hand";
    DrawRectangleRounded(BIH_REQ_RECT, 0.3, 3, CLOTH_COLOR);
    DrawText(
        btn_text,
        WIN_W / 2 - strlen(btn_text) * btn_font_size / 3,
        BIH_REQ_RECT.y + BIH_REQ_RECT.height / 4,
        btn_font_size,
        RAYWHITE
    );
    if (CheckCollisionPointRec(GetMousePosition(), BIH_REQ_RECT) &&
        game_state == STATE_HITTING) {
        DrawRectangleRoundedLines(BIH_REQ_RECT, 0.3, 5, 2, RAYWHITE);
        // Check for "pressed" so that we can react before other game mechanics
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            game_state = STATE_BALL_IN_HAND;
    }
    // Don't draw hit hint when on button
    else if (game_state == STATE_HITTING) {
        float hint_outer_r = BALL_R *
            Lerp(HINT_LEN_MIN,
                 HINT_LEN_MAX,
                 (hit_power - HIT_POWER_MIN) / HIT_POWER_MAX);
#ifdef DEBUG
        hint_outer_r *= 6;
#endif
        DrawLineEx(
            Vector2Add(cue_ball.pos, Vector2Scale(cue_dir, -BALL_R * 1.5)),
            Vector2Add(cue_ball.pos, Vector2Scale(cue_dir, -hint_outer_r)),
            3,
            RAYWHITE
        );
    }

#ifdef DEBUG
    DrawFPS(5, 5);
#endif
    EndDrawing();
}

// Check if ball is pocketed or near enough a specific pocket to be drawn in
// Return true if the ball was near the pocket
bool check_pocket(Ball *ball, size_t pocket_x, size_t pocket_y) {
    Vector2 pocket_pos = {
        pocket_xs[pocket_x],
        pocket_x == 1 ? center_pocket_check_ys[pocket_y] : pocket_ys[pocket_y]
    };

    Vector2 pocket_dir = Vector2Subtract(pocket_pos, ball->pos);
    float velocity_size = Vector2Length(ball->vel);
    float pocket_dis = Vector2Length(pocket_dir);
    pocket_dir = Vector2Normalize(pocket_dir);

    // Ball might be pocketed if it's fast but actually in the
    // pocket, so this condition is put first
    if (pocket_dis < POCKET_ZONE_MIN_DISTANCE) {
        utl_log(UTL_DEBUG, "pocket distance: %f", pocket_dis);
        ball->is_pocketed = true;
        return true;
    }
    if (velocity_size > POCKET_MAX_VELOCITY ||
        pocket_dis > POCKET_ZONE_MAX_DISTANCE)
        return false;
    // Ball we'll get attracted to the pocket in pocket zone
    ball->vel =
        Vector2Scale(pocket_dir, fmax(velocity_size, POCKET_MIN_VELOCITY));
    return true;
}

void update_physics() {
    // A number which each of its bits represent if one of the balls is
    // moving
    bool motion_exists = false;

    for (size_t i = 0; i <= BALL_COUNT; i++) {
        Ball *b = &balls[i];
        if (b->is_pocketed) continue;

        // Update position
        Vector2 acc = Vector2Scale(b->vel, -CLOTH_DRAG);
        mot_adaptive_verlet(&b->pos, &b->vel, acc, b->prev_acc, GetFrameTime());
        b->prev_acc = acc;

        // Check if ball is near any pocket
        bool pocket_zoned = false;
        for (size_t j = 0; j < 6; j++) {
            if (check_pocket(b, j % 3, j / 3)) {
                pocket_zoned = true;
                break;
            }
        }

        if (pocket_zoned) {
            motion_exists = true;
            // Skip collision check so that the ball can get into pockets
            continue;
        }

        // Boundary collision check
        if (b->pos.x > CLOTH_RECT.x + CLOTH_RECT.width - BALL_R) {
            b->vel.x *= -1;
            b->pos.x = CLOTH_RECT.x + CLOTH_RECT.width - BALL_R;
        } else if (b->pos.x < CLOTH_RECT.x + BALL_R) {
            b->vel.x *= -1;
            b->pos.x = CLOTH_RECT.x + BALL_R;
        }
        if (b->pos.y > CLOTH_RECT.y + CLOTH_RECT.height - BALL_R) {
            b->vel.y *= -1;
            b->pos.y = CLOTH_RECT.y + CLOTH_RECT.height - BALL_R;
        } else if (b->pos.y < CLOTH_RECT.y + BALL_R) {
            b->vel.y *= -1;
            b->pos.y = CLOTH_RECT.y + BALL_R;
        }

        bool is_moving = Vector2LengthSqr(b->vel) > VELOCITY_SQR_MIN;
        motion_exists = motion_exists || is_moving;
        if (!is_moving) continue;

        // Check collision with other balls
        for (size_t j = 0; j <= BALL_COUNT; j++) {
            if (i == j) continue;
            Ball *b1 = &balls[j];
            Vector2 delta = Vector2Subtract(b->pos, b1->pos);
            float distance = Vector2Length(delta);

            if (distance > 2 * BALL_R) continue;

            Vector2 norm = Vector2Normalize(delta);
            Vector2 rel_velocity = Vector2Subtract(b->vel, b1->vel);
            Vector2 impulse =
                Vector2Scale(norm, Vector2DotProduct(rel_velocity, norm));

            b1->vel = Vector2Add(b1->vel, impulse);
            b->vel = Vector2Subtract(b->vel, impulse);

            // Position correction
            float overlap = BALL_R * 2 - distance;
            Vector2 correction = Vector2Scale(norm, overlap / 2);
            b->pos = Vector2Add(b->pos, correction);
            b1->pos = Vector2Subtract(b1->pos, correction);
        }
    }

    if (!motion_exists) {
        if (balls[BALL_COUNT].is_pocketed) {  // (cue ball is pocketed)
            balls[BALL_COUNT].is_pocketed = false;
            game_state = STATE_BALL_IN_HAND;
            return;
        }
        game_state = STATE_HITTING;
    }
}

void update_frame(void) {
    // Hit the ball
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
        game_state == STATE_HITTING) {
        Vector2 cue_dir = Vector2Normalize(
            Vector2Subtract(GetMousePosition(), balls[BALL_COUNT].pos)
        );
        balls[BALL_COUNT].vel = Vector2Scale(cue_dir, -hit_power);
        hit_power = HIT_POWER_MAX / 2;
        game_state = STATE_MOTION;
    }

    // Aim the ball
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && game_state == STATE_HITTING) {
        float hit_distance = Clamp(
            Vector2Distance(hit_mouse_pos, GetMousePosition()),
            0,
            HIT_DISTANCE_MAX
        );
        hit_power =
            Lerp(HIT_POWER_MIN, HIT_POWER_MAX, hit_distance / HIT_DISTANCE_MAX);
    }

    // Set up aiming and hitting
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        hit_mouse_pos = GetMousePosition();
        if (game_state == STATE_STARTING || game_state == STATE_BALL_IN_HAND)
            game_state = STATE_HITTING;
    }

    // Initial ball placement for the break
    if (game_state == STATE_STARTING) {
        Vector2 mouse_pos = GetMousePosition();
        Vector2 cue_pos = {
            Clamp(mouse_pos.x, CLOTH_RECT.x, HEADSTRING_X),
            Clamp(mouse_pos.y, CLOTH_RECT.y, CLOTH_RECT.y + CLOTH_RECT.height)
        };
        balls[BALL_COUNT].pos = cue_pos;
    }

    if (game_state == STATE_BALL_IN_HAND) {
        Vector2 mouse_pos = GetMousePosition();
        Vector2 cue_pos = {
            Clamp(mouse_pos.x, CLOTH_RECT.x, CLOTH_RECT.x + CLOTH_RECT.width),
            Clamp(mouse_pos.y, CLOTH_RECT.y, CLOTH_RECT.y + CLOTH_RECT.height)
        };
        balls[BALL_COUNT].pos = cue_pos;
    }

    if (game_state == STATE_MOTION) update_physics();
    render();
}

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WIN_W, WIN_H, "8 Ball");
    SetTargetFPS(FPS);

    pocket_ys[0] = CLOTH_RECT.y + POCKET_R / 2;                      // top y
    pocket_ys[1] = CLOTH_RECT.y + CLOTH_RECT.height - POCKET_R / 2;  // bottom y
    pocket_xs[0] = CLOTH_RECT.x + POCKET_R / 2;                      // left x
    pocket_xs[1] = WIN_W / 2;                                        // center x
    pocket_xs[2] = CLOTH_RECT.x + CLOTH_RECT.width - POCKET_R / 2;   // right x
    center_pocket_check_ys[0] = CLOTH_RECT.y;                        // top y
    center_pocket_check_ys[1] = CLOTH_RECT.y + CLOTH_RECT.height;    // bottom y

    init_balls();

    rayutl_mainloop(update_frame, FPS);

    CloseWindow();
    return 0;
}