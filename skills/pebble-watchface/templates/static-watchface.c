/**
 * Static/Analog Pebble Watchface Template
 *
 * This template provides a foundation for creating static watchfaces
 * including analog clock designs. Optimized for battery efficiency
 * with minute-based updates.
 *
 * Customize the drawing functions to create your unique design.
 */

#include <pebble.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define WATCHFACE_NAME "My Static Watch"

// Clock configuration
#define CLOCK_RADIUS 60
#define HOUR_HAND_LENGTH 35
#define MINUTE_HAND_LENGTH 50
#define SECOND_HAND_LENGTH 55
#define SHOW_SECOND_HAND false  // Set to true for second hand (uses more battery)

// ============================================================================
// GLOBAL STATE
// ============================================================================

static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_date_layer;
static Layer *s_battery_layer;

static int s_battery_level = 100;

// Clock center (calculated in window_load)
static GPoint s_center;

// Pre-allocated paths for clock hands
static GPath *s_hour_hand_path = NULL;
static GPath *s_minute_hand_path = NULL;

static GPoint s_hour_hand_points[4];
static GPoint s_minute_hand_points[4];

static GPathInfo s_hour_hand_info = {
    .num_points = 4,
    .points = s_hour_hand_points
};

static GPathInfo s_minute_hand_info = {
    .num_points = 4,
    .points = s_minute_hand_points
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static void calculate_hand_points(GPoint *points, int length, int width) {
    // Diamond-shaped hand pointing up (will be rotated)
    points[0] = GPoint(0, -length);          // Tip
    points[1] = GPoint(width, -length/3);    // Right side
    points[2] = GPoint(0, length/5);         // Bottom
    points[3] = GPoint(-width, -length/3);   // Left side
}

// ============================================================================
// DRAWING FUNCTIONS
// ============================================================================

static void draw_clock_face(GContext *ctx) {
    // Outer circle
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, s_center, CLOCK_RADIUS);

    // Hour markers
    for (int i = 0; i < 12; i++) {
        int32_t angle = (i * TRIG_MAX_ANGLE) / 12;

        int marker_length = (i % 3 == 0) ? 10 : 5;  // Longer at 12, 3, 6, 9
        int inner_r = CLOCK_RADIUS - marker_length;
        int outer_r = CLOCK_RADIUS - 2;

        GPoint inner = {
            s_center.x + (sin_lookup(angle) * inner_r) / TRIG_MAX_RATIO,
            s_center.y - (cos_lookup(angle) * inner_r) / TRIG_MAX_RATIO
        };
        GPoint outer = {
            s_center.x + (sin_lookup(angle) * outer_r) / TRIG_MAX_RATIO,
            s_center.y - (cos_lookup(angle) * outer_r) / TRIG_MAX_RATIO
        };

        graphics_context_set_stroke_width(ctx, (i % 3 == 0) ? 3 : 1);
        graphics_draw_line(ctx, inner, outer);
    }
}

static void draw_clock_hand(GContext *ctx, GPath *path, int32_t angle) {
    gpath_rotate_to(path, angle);
    gpath_move_to(path, s_center);

    graphics_context_set_fill_color(ctx, GColorWhite);
    gpath_draw_filled(ctx, path);

    graphics_context_set_stroke_color(ctx, GColorBlack);
    gpath_draw_outline(ctx, path);
}

static void draw_hands(GContext *ctx, struct tm *time) {
    // Hour hand
    int32_t hour_angle = ((time->tm_hour % 12) * TRIG_MAX_ANGLE / 12) +
                         (time->tm_min * TRIG_MAX_ANGLE / 12 / 60);
    draw_clock_hand(ctx, s_hour_hand_path, hour_angle);

    // Minute hand
    int32_t minute_angle = (time->tm_min * TRIG_MAX_ANGLE / 60) +
                           (time->tm_sec * TRIG_MAX_ANGLE / 60 / 60);
    draw_clock_hand(ctx, s_minute_hand_path, minute_angle);

    // Second hand (optional - uses more battery)
    #if SHOW_SECOND_HAND
    int32_t second_angle = (time->tm_sec * TRIG_MAX_ANGLE / 60);
    int16_t sec_x = s_center.x + (sin_lookup(second_angle) * SECOND_HAND_LENGTH) / TRIG_MAX_RATIO;
    int16_t sec_y = s_center.y - (cos_lookup(second_angle) * SECOND_HAND_LENGTH) / TRIG_MAX_RATIO;

    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, s_center, GPoint(sec_x, sec_y));
    #endif

    // Center dot
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, s_center, 4);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, s_center, 2);
}

static void draw_decorations(GContext *ctx, GRect bounds) {
    // Add any decorative elements here
    // Example: Draw a simple border

    #ifdef PBL_ROUND
    // Round display decorations
    (void)bounds;
    #else
    // Rectangular display decorations
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 1);

    // Corner accents — use dynamic bounds
    int w = bounds.size.w - 5;
    int h = bounds.size.h - 5;
    graphics_draw_line(ctx, GPoint(5, 5), GPoint(20, 5));
    graphics_draw_line(ctx, GPoint(5, 5), GPoint(5, 20));

    graphics_draw_line(ctx, GPoint(w, 5), GPoint(w - 15, 5));
    graphics_draw_line(ctx, GPoint(w, 5), GPoint(w, 20));

    graphics_draw_line(ctx, GPoint(5, h), GPoint(20, h));
    graphics_draw_line(ctx, GPoint(5, h), GPoint(5, h - 15));

    graphics_draw_line(ctx, GPoint(w, h), GPoint(w - 15, h));
    graphics_draw_line(ctx, GPoint(w, h), GPoint(w, h - 15));
    #endif
}

// ============================================================================
// LAYER UPDATE PROCEDURES
// ============================================================================

static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    // Clear background
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // Draw decorations (background)
    draw_decorations(ctx, bounds);

    // Draw clock face
    draw_clock_face(ctx);

    // Get current time and draw hands
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    if (tick_time) {
        draw_hands(ctx, tick_time);
    }
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    const int WIDTH = 20;
    const int HEIGHT = 8;

    // Outline
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_rect(ctx, GRect(0, 0, WIDTH, HEIGHT));

    // Fill
    int fill_width = (s_battery_level * WIDTH) / 100;
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(0, 0, fill_width, HEIGHT), 0, GCornerNone);

    // Battery tip
    graphics_fill_rect(ctx, GRect(WIDTH, 2, 2, HEIGHT - 4), 0, GCornerNone);
}

// ============================================================================
// TIME HANDLING
// ============================================================================

static void update_display(void) {
    // Update date
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    if (tick_time && s_date_layer) {
        static char date_buffer[16];
        strftime(date_buffer, sizeof(date_buffer), "%a %d", tick_time);
        text_layer_set_text(s_date_layer, date_buffer);
    }

    // Request canvas redraw
    if (s_canvas_layer) {
        layer_mark_dirty(s_canvas_layer);
    }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_display();
}

// ============================================================================
// BATTERY HANDLING
// ============================================================================

static void battery_callback(BatteryChargeState state) {
    s_battery_level = state.charge_percent;

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

    // Calculate center
    s_center = GPoint(bounds.size.w / 2, bounds.size.h / 2);

    // Canvas layer
    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);

    // Date layer (below the clock)
    GRect date_frame = {{0, bounds.size.h - 30}, {bounds.size.w, 20}};
    s_date_layer = text_layer_create(date_frame);
    text_layer_set_text_color(s_date_layer, GColorWhite);
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    // Battery layer
    GRect battery_frame = {{bounds.size.w - 27, 5}, {22, 8}};
    s_battery_layer = layer_create(battery_frame);
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    layer_add_child(window_layer, s_battery_layer);

    // Initialize hand paths
    calculate_hand_points(s_hour_hand_points, HOUR_HAND_LENGTH, 4);
    calculate_hand_points(s_minute_hand_points, MINUTE_HAND_LENGTH, 3);

    s_hour_hand_path = gpath_create(&s_hour_hand_info);
    s_minute_hand_path = gpath_create(&s_minute_hand_info);

    // Initial display update
    update_display();
}

static void main_window_unload(Window *window) {
    // Destroy paths
    if (s_hour_hand_path) {
        gpath_destroy(s_hour_hand_path);
        s_hour_hand_path = NULL;
    }
    if (s_minute_hand_path) {
        gpath_destroy(s_minute_hand_path);
        s_minute_hand_path = NULL;
    }

    // Destroy layers
    if (s_canvas_layer) {
        layer_destroy(s_canvas_layer);
        s_canvas_layer = NULL;
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
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    window_stack_push(s_main_window, true);

    // Subscribe to time updates
    #if SHOW_SECOND_HAND
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    #else
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    #endif

    battery_state_service_subscribe(battery_callback);
    s_battery_level = battery_state_service_peek().charge_percent;
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
