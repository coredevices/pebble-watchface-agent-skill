# Animation Patterns for Pebble Watchfaces

## Animation Loop Structure

### Basic Animation Timer
```c
static AppTimer *s_animation_timer = NULL;
#define ANIMATION_INTERVAL 50  // 20 FPS

static void animation_timer_callback(void *data) {
    // Update animation state
    update_animation();

    // Request redraw
    layer_mark_dirty(s_canvas_layer);

    // Schedule next frame
    s_animation_timer = app_timer_register(ANIMATION_INTERVAL, animation_timer_callback, NULL);
}

// Start in window_load:
s_animation_timer = app_timer_register(ANIMATION_INTERVAL, animation_timer_callback, NULL);

// Cancel in window_unload:
if (s_animation_timer) {
    app_timer_cancel(s_animation_timer);
    s_animation_timer = NULL;
}
```

### Battery-Aware Animation
```c
#define ANIMATION_INTERVAL 50
#define ANIMATION_INTERVAL_LOW_POWER 100
#define LOW_BATTERY_THRESHOLD 20

static int s_battery_level = 100;

static void animation_timer_callback(void *data) {
    update_animation();
    layer_mark_dirty(s_canvas_layer);

    uint32_t interval = (s_battery_level <= LOW_BATTERY_THRESHOLD)
                        ? ANIMATION_INTERVAL_LOW_POWER
                        : ANIMATION_INTERVAL;

    s_animation_timer = app_timer_register(interval, animation_timer_callback, NULL);
}

static void battery_callback(BatteryChargeState state) {
    s_battery_level = state.charge_percent;
}
```

## Common Animation Patterns

### Oscillating Motion (Wave/Sway)
```c
typedef struct {
    GPoint pos;
    int32_t phase;  // Animation phase
    int speed;
} Swaying;

static void update_sway(Swaying *obj) {
    // Increment phase with overflow protection
    obj->phase = (obj->phase + obj->speed * 100) % TRIG_MAX_ANGLE;
}

static void draw_sway(GContext *ctx, Swaying *obj) {
    // Calculate offset using sine
    int16_t offset = (sin_lookup(obj->phase) * 5) / TRIG_MAX_RATIO;

    GPoint draw_pos = {
        .x = obj->pos.x + offset,
        .y = obj->pos.y
    };

    graphics_fill_circle(ctx, draw_pos, 5);
}
```

### Linear Movement with Wrapping
```c
typedef struct {
    GPoint pos;
    int direction;  // 1 or -1
    int speed;
    bool active;
} Moving;

// Use screen_w and screen_h from layer_get_bounds() — don't hardcode!
static void update_moving(Moving *obj, int screen_w, int screen_h) {
    if (!obj->active) return;

    obj->pos.x += obj->direction * obj->speed;

    // Wrap around screen edges (use dynamic screen dimensions)
    if (obj->direction == 1 && obj->pos.x > screen_w + 10) {
        obj->pos.x = -10;
        obj->pos.y = random_in_range(20, screen_h - 30);
    } else if (obj->direction == -1 && obj->pos.x < -10) {
        obj->pos.x = screen_w + 10;
        obj->pos.y = random_in_range(20, screen_h - 30);
    }
}
```

### Pulsing Effect
```c
typedef struct {
    GPoint pos;
    int pulse_state;  // 0 to 100
    int base_size;
} Pulsing;

static void update_pulse(Pulsing *obj) {
    obj->pulse_state = (obj->pulse_state + 2) % 100;
}

static void draw_pulse(GContext *ctx, Pulsing *obj) {
    // Size oscillates between base_size and base_size + 4
    int size_offset = (obj->pulse_state < 50)
                      ? obj->pulse_state / 10
                      : (100 - obj->pulse_state) / 10;

    int size = obj->base_size + size_offset;
    graphics_fill_circle(ctx, obj->pos, size);
}
```

### Rising Particles (Bubbles)
```c
typedef struct {
    GPoint pos;
    int size;
    int speed;
    bool active;
} Particle;

#define MAX_PARTICLES 8
static Particle particles[MAX_PARTICLES];

// Pass screen dimensions from layer_get_bounds()
static void init_particle(Particle *p, int screen_w, int screen_h) {
    p->pos.x = random_in_range(10, screen_w - 10);
    p->pos.y = screen_h;  // Start at bottom
    p->size = random_in_range(1, 3);
    p->speed = random_in_range(1, 3);
    p->active = true;
}

static void update_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].pos.y -= particles[i].speed;

            // Slight horizontal wobble
            if (random_in_range(0, 2) == 0) {
                particles[i].pos.x += random_in_range(-1, 1);
            }

            // Deactivate when off screen
            if (particles[i].pos.y < 0) {
                particles[i].active = false;
            }
        } else {
            // Random chance to spawn new particle
            if (random_in_range(0, 100) < 2) {
                init_particle(&particles[i]);
            }
        }
    }
}
```

### Tentacle/Wavy Line Animation
```c
static void draw_wavy_line(GContext *ctx, GPoint start, int length,
                           int segments, int32_t phase) {
    GPoint current = start;
    GPoint next;

    for (int i = 0; i < segments; i++) {
        int32_t angle = (phase + (i * 1500)) % TRIG_MAX_ANGLE;
        int16_t offset = (sin_lookup(angle) * 3) / TRIG_MAX_RATIO;

        next.x = current.x + offset;
        next.y = current.y + (length / segments);

        graphics_draw_line(ctx, current, next);
        current = next;
    }
}
```

## Collision Detection

### Circle-Circle Collision
```c
static bool check_collision(GPoint pos1, int radius1, GPoint pos2, int radius2) {
    int dx = pos1.x - pos2.x;
    int dy = pos1.y - pos2.y;
    int distance_squared = (dx * dx) + (dy * dy);
    int radius_sum = radius1 + radius2;
    return distance_squared <= (radius_sum * radius_sum);
}
```

### Spatial Grid Optimization
For many moving objects, use a spatial grid to reduce collision checks from O(n²) to O(n):

```c
// Use dynamic screen dimensions from layer_get_bounds()
#define GRID_WIDTH 3
#define GRID_HEIGHT 3
// Calculate cell sizes at runtime using bounds.size.w / GRID_WIDTH
#define GRID_CELL_COUNT (GRID_WIDTH * GRID_HEIGHT)

static int objects_in_grid[GRID_CELL_COUNT][MAX_OBJECTS];
static int grid_counts[GRID_CELL_COUNT];

static int get_grid_cell(GPoint point) {
    int x = point.x / GRID_CELL_WIDTH;
    int y = point.y / GRID_CELL_HEIGHT;

    // Clamp to valid range
    if (x < 0) x = 0;
    if (x >= GRID_WIDTH) x = GRID_WIDTH - 1;
    if (y < 0) y = 0;
    if (y >= GRID_HEIGHT) y = GRID_HEIGHT - 1;

    return y * GRID_WIDTH + x;
}

static void update_spatial_grid(void) {
    // Clear grid
    for (int i = 0; i < GRID_CELL_COUNT; i++) {
        grid_counts[i] = 0;
    }

    // Place objects in cells
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].active) {
            int cell = get_grid_cell(objects[i].pos);
            if (grid_counts[cell] < MAX_OBJECTS) {
                objects_in_grid[cell][grid_counts[cell]] = i;
                grid_counts[cell]++;
            }
        }
    }
}

// Only check collisions with objects in same/adjacent cells
static void check_collisions_optimized(int object_index) {
    int cell = get_grid_cell(objects[object_index].pos);

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int check_x = (cell % GRID_WIDTH) + dx;
            int check_y = (cell / GRID_WIDTH) + dy;

            if (check_x < 0 || check_x >= GRID_WIDTH) continue;
            if (check_y < 0 || check_y >= GRID_HEIGHT) continue;

            int check_cell = check_y * GRID_WIDTH + check_x;

            for (int i = 0; i < grid_counts[check_cell]; i++) {
                int other = objects_in_grid[check_cell][i];
                if (other == object_index) continue;

                if (check_collision(objects[object_index].pos, 5,
                                   objects[other].pos, 5)) {
                    // Handle collision
                }
            }
        }
    }
}
```

## Analog Clock Hands

### Drawing Clock Hands
```c
static void draw_hand(GContext *ctx, GPoint center, int length,
                      int32_t angle, int width) {
    int16_t x = center.x + (sin_lookup(angle) * length) / TRIG_MAX_RATIO;
    int16_t y = center.y - (cos_lookup(angle) * length) / TRIG_MAX_RATIO;

    graphics_context_set_stroke_width(ctx, width);
    graphics_draw_line(ctx, center, GPoint(x, y));
}

static void draw_clock_hands(GContext *ctx, struct tm *time, GPoint center) {
    // Hour hand
    int32_t hour_angle = ((time->tm_hour % 12) * TRIG_MAX_ANGLE / 12) +
                         (time->tm_min * TRIG_MAX_ANGLE / 12 / 60);
    draw_hand(ctx, center, 30, hour_angle, 4);

    // Minute hand
    int32_t min_angle = (time->tm_min * TRIG_MAX_ANGLE / 60);
    draw_hand(ctx, center, 45, min_angle, 2);

    // Second hand
    int32_t sec_angle = (time->tm_sec * TRIG_MAX_ANGLE / 60);
    draw_hand(ctx, center, 50, sec_angle, 1);

    // Center dot
    graphics_fill_circle(ctx, center, 4);
}
```

## Memory-Efficient Path Animation

Pre-allocate path objects and update points in-place:

```c
static GPoint tail_points[3];
static GPath *tail_path = NULL;

// In window_load:
static GPathInfo tail_info = {
    .num_points = 3,
    .points = tail_points
};
tail_path = gpath_create(&tail_info);

// In draw function (update points, don't recreate path):
static void draw_fish_tail(GContext *ctx, GPoint pos, int direction) {
    tail_points[0].x = pos.x;
    tail_points[0].y = pos.y;
    tail_points[1].x = pos.x - (direction * 10);
    tail_points[1].y = pos.y - 5;
    tail_points[2].x = pos.x - (direction * 10);
    tail_points[2].y = pos.y + 5;

    gpath_draw_filled(ctx, tail_path);
}

// In window_unload:
if (tail_path) {
    gpath_destroy(tail_path);
    tail_path = NULL;
}
```

## State Machine Animation

For complex animations with multiple states:

```c
typedef enum {
    STATE_IDLE,
    STATE_MOVING,
    STATE_ATTACKING,
    STATE_FLEEING
} AnimState;

typedef struct {
    GPoint pos;
    AnimState state;
    int state_timer;
    int direction;
} Creature;

// Pass screen_w from layer_get_bounds().size.w
static void update_creature(Creature *c, int screen_w) {
    c->state_timer++;

    switch (c->state) {
        case STATE_IDLE:
            if (c->state_timer > 100) {
                c->state = STATE_MOVING;
                c->state_timer = 0;
                c->direction = random_in_range(0, 1) ? 1 : -1;
            }
            break;

        case STATE_MOVING:
            c->pos.x += c->direction * 2;
            if (c->state_timer > 50 || c->pos.x < 10 || c->pos.x > screen_w - 10) {
                c->state = STATE_IDLE;
                c->state_timer = 0;
            }
            break;

        // ... other states
    }
}
```
