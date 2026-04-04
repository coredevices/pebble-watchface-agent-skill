/**
 * Animated Pebble Watchface Template
 *
 * This template provides a foundation for creating animated watchfaces
 * with multiple moving elements, efficient memory management, and
 * battery-aware animation throttling.
 *
 * Customize the animated elements, drawing functions, and update logic
 * to create your unique watchface design.
 */

#include <pebble.h>

// ============================================================================
// CONFIGURATION - Customize these values
// ============================================================================

#define WATCHFACE_NAME "My Animated Watch"

// Animation settings
#define ANIMATION_INTERVAL 50           // Normal: 50ms = 20 FPS
#define ANIMATION_INTERVAL_LOW_POWER 100 // Low battery: 100ms = 10 FPS
#define LOW_BATTERY_THRESHOLD 20        // Throttle below 20%

// Element counts - adjust based on your design
#define MAX_PARTICLES 8
#define MAX_MOVING_OBJECTS 4

// ============================================================================
// DATA STRUCTURES - Define your animated elements
// ============================================================================

typedef struct {
    GPoint pos;
    int direction;  // 1 or -1
    int speed;
    bool active;
} MovingObject;

typedef struct {
    GPoint pos;
    int size;
    int speed;
    bool active;
} Particle;

// ============================================================================
// GLOBAL STATE
// ============================================================================

// UI Elements
static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static Layer *s_battery_layer;
static AppTimer *s_animation_timer;

// Battery state
static int s_battery_level = 100;
static bool s_is_charging = false;

// Animated elements
static MovingObject s_objects[MAX_MOVING_OBJECTS];
static Particle s_particles[MAX_PARTICLES];

// Animation state
static int32_t s_animation_phase = 0;

// Pre-allocated paths (for complex shapes)
static GPath *s_shape_path = NULL;
static GPoint s_shape_points[4];
static GPathInfo s_shape_info = {
    .num_points = 4,
    .points = s_shape_points
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static int random_in_range(int min, int max) {
    if (max <= min) return min;
    int range = max - min + 1;
    return min + (rand() % range);
}

// ============================================================================
// INITIALIZATION FUNCTIONS
// ============================================================================

// Screen dimensions — set in window_load from layer_get_bounds()
static int s_screen_w = 200;
static int s_screen_h = 228;

static void init_moving_object(MovingObject *obj) {
    obj->pos.y = random_in_range(30, s_screen_h - 40);
    obj->direction = (random_in_range(0, 1) * 2) - 1;
    obj->speed = random_in_range(1, 3);
    obj->pos.x = (obj->direction == 1) ? -10 : s_screen_w + 10;
    obj->active = true;
}

static void init_particle(Particle *p) {
    p->pos.x = random_in_range(10, s_screen_w - 10);
    p->pos.y = s_screen_h;  // Start at bottom
    p->size = random_in_range(1, 3);
    p->speed = random_in_range(1, 3);
    p->active = true;
}

// ============================================================================
// DRAWING FUNCTIONS - Customize your visuals here
// ============================================================================

static void draw_moving_object(GContext *ctx, const MovingObject *obj) {
    if (!obj || !obj->active) return;

    graphics_context_set_fill_color(ctx, GColorWhite);

    // Example: Draw a simple circle
    graphics_fill_circle(ctx, obj->pos, 5);

    // Example: Draw a directional tail
    GPoint tail_end = {
        obj->pos.x - (obj->direction * 10),
        obj->pos.y
    };
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx, obj->pos, tail_end);
}

static void draw_particle(GContext *ctx, const Particle *p) {
    if (!p || !p->active) return;

    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_circle(ctx, p->pos, p->size);
}

static void draw_background_element(GContext *ctx, int32_t phase) {
    // Example: Oscillating background element
    int16_t offset = (sin_lookup(phase) * 10) / TRIG_MAX_RATIO;

    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 2);

    GPoint start = {20, 160};
    GPoint end = {20 + offset, 140};
    graphics_draw_line(ctx, start, end);

    // Add more background elements as needed
}

// ============================================================================
// UPDATE FUNCTIONS
// ============================================================================

static void update_moving_objects(void) {
    for (int i = 0; i < MAX_MOVING_OBJECTS; i++) {
        if (!s_objects[i].active) continue;

        s_objects[i].pos.x += s_objects[i].direction * s_objects[i].speed;

        // Reset when off screen
        if ((s_objects[i].direction == 1 && s_objects[i].pos.x > s_screen_w + 10) ||
            (s_objects[i].direction == -1 && s_objects[i].pos.x < -10)) {
            init_moving_object(&s_objects[i]);
        }
    }
}

static void update_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (s_particles[i].active) {
            s_particles[i].pos.y -= s_particles[i].speed;

            // Slight horizontal wobble
            if (random_in_range(0, 2) == 0) {
                s_particles[i].pos.x += random_in_range(-1, 1);
            }

            // Deactivate when off screen
            if (s_particles[i].pos.y < 0) {
                s_particles[i].active = false;
            }
        } else {
            // Random chance to spawn
            if (random_in_range(0, 100) < 2) {
                init_particle(&s_particles[i]);
            }
        }
    }
}

static void animation_update(void) {
    // Update animation phase with overflow protection
    s_animation_phase = (s_animation_phase + 200) % TRIG_MAX_ANGLE;

    // Update all animated elements
    update_moving_objects();
    update_particles();

    // Request redraw
    if (s_canvas_layer) {
        layer_mark_dirty(s_canvas_layer);
    }
}

// ============================================================================
// LAYER UPDATE PROCEDURES
// ============================================================================

static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    // Clear background
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // Draw background elements
    draw_background_element(ctx, s_animation_phase);

    // Draw particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        draw_particle(ctx, &s_particles[i]);
    }

    // Draw moving objects
    for (int i = 0; i < MAX_MOVING_OBJECTS; i++) {
        draw_moving_object(ctx, &s_objects[i]);
    }
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    BatteryChargeState state = battery_state_service_peek();

    const int WIDTH = 20;
    const int HEIGHT = 8;
    GRect outline = {{0, 0}, {WIDTH, HEIGHT}};

    // Outline
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_rect(ctx, outline);

    // Fill based on level
    int fill_width = (state.charge_percent * WIDTH) / 100;
    GRect fill = {{0, 0}, {fill_width, HEIGHT}};
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, fill, 0, GCornerNone);
}

// ============================================================================
// TIME HANDLING
// ============================================================================

static void update_time(void) {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    if (!tick_time || !s_time_layer || !s_date_layer) return;

    static char time_buffer[8];
    strftime(time_buffer, sizeof(time_buffer), "%I:%M", tick_time);
    text_layer_set_text(s_time_layer, time_buffer);

    static char date_buffer[24];
    strftime(date_buffer, sizeof(date_buffer), "%a, %b %d", tick_time);
    text_layer_set_text(s_date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
}

// ============================================================================
// TIMER HANDLING
// ============================================================================

static void animation_timer_callback(void *data) {
    animation_update();

    // Schedule next frame (battery-aware)
    uint32_t interval = (s_battery_level <= LOW_BATTERY_THRESHOLD && !s_is_charging)
                        ? ANIMATION_INTERVAL_LOW_POWER
                        : ANIMATION_INTERVAL;

    s_animation_timer = app_timer_register(interval, animation_timer_callback, NULL);
}

// ============================================================================
// BATTERY HANDLING
// ============================================================================

static void battery_callback(BatteryChargeState state) {
    s_battery_level = state.charge_percent;
    s_is_charging = state.is_charging;

    if (s_battery_layer) {
        layer_mark_dirty(s_battery_layer);
    }
}

// ============================================================================
// WINDOW HANDLERS
// ============================================================================

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Store screen dimensions for animation calculations
    s_screen_w = bounds.size.w;
    s_screen_h = bounds.size.h;

    // Canvas layer (full screen for animations)
    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);

    // Time layer
    GRect time_frame = {{0, 50}, {bounds.size.w, 34}};
    s_time_layer = text_layer_create(time_frame);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

    // Date layer
    GRect date_frame = {{0, 84}, {bounds.size.w, 20}};
    s_date_layer = text_layer_create(date_frame);
    text_layer_set_text_color(s_date_layer, GColorWhite);
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    // Battery layer
    GRect battery_frame = {{bounds.size.w - 25, 5}, {20, 8}};
    s_battery_layer = layer_create(battery_frame);
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    layer_add_child(window_layer, s_battery_layer);

    // Initialize animated elements
    for (int i = 0; i < MAX_MOVING_OBJECTS; i++) {
        init_moving_object(&s_objects[i]);
    }
    for (int i = 0; i < MAX_PARTICLES; i++) {
        s_particles[i].active = false;
    }

    // Create pre-allocated paths
    s_shape_path = gpath_create(&s_shape_info);

    // Start animation timer
    s_animation_timer = app_timer_register(ANIMATION_INTERVAL, animation_timer_callback, NULL);

    // Initial time update
    update_time();
}

static void main_window_unload(Window *window) {
    // Cancel animation timer
    if (s_animation_timer) {
        app_timer_cancel(s_animation_timer);
        s_animation_timer = NULL;
    }

    // Destroy paths
    if (s_shape_path) {
        gpath_destroy(s_shape_path);
        s_shape_path = NULL;
    }

    // Destroy layers
    if (s_canvas_layer) {
        layer_destroy(s_canvas_layer);
        s_canvas_layer = NULL;
    }
    if (s_time_layer) {
        text_layer_destroy(s_time_layer);
        s_time_layer = NULL;
    }
    if (s_date_layer) {
        text_layer_destroy(s_date_layer);
        s_date_layer = NULL;
    }
    if (s_battery_layer) {
        layer_destroy(s_battery_layer);
        s_battery_layer = NULL;
    }
}

// ============================================================================
// APPLICATION LIFECYCLE
// ============================================================================

static void init(void) {
    srand(time(NULL));

    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    window_stack_push(s_main_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_callback);

    // Get initial battery state
    BatteryChargeState state = battery_state_service_peek();
    s_battery_level = state.charge_percent;
    s_is_charging = state.is_charging;
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();

    if (s_main_window) {
        window_destroy(s_main_window);
        s_main_window = NULL;
    }
}

int main(void) {
    init();
    app_event_loop();
    deinit();
    return 0;
}
