#include <pebble.h>

// ============================================================================
// COLORS
// ============================================================================

#ifdef PBL_COLOR
  #define COLOR_BG              GColorBlack
  #define COLOR_STARS           GColorWhite
  #define COLOR_STARS_DIM       GColorLightGray
  #define COLOR_SABER_RED       GColorRed
  #define COLOR_SABER_RED_GLOW  GColorDarkCandyAppleRed
  #define COLOR_SABER_GREEN     GColorGreen
  #define COLOR_SABER_GREEN_GLOW GColorIslamicGreen
  #define COLOR_SABER_CORE      GColorWhite
  #define COLOR_HILT            GColorLightGray
  #define COLOR_HILT_DARK       GColorDarkGray
  #define COLOR_VADER_CAPE      GColorDarkGray
  #define COLOR_VADER_OUTLINE   GColorDarkGray
  #define COLOR_LUKE_BODY       GColorDarkGray
  #define COLOR_LUKE_SKIN       GColorMelon
  #define COLOR_TIME_TEXT       GColorYellow
  #define COLOR_DATE_TEXT       GColorCeleste
  #define COLOR_HOUR_MARKERS    GColorDarkGray
  #define COLOR_CENTER_DOT      GColorYellow
  #define COLOR_BATTERY_GOOD    GColorGreen
  #define COLOR_BATTERY_MED     GColorYellow
  #define COLOR_BATTERY_LOW     GColorRed
#else
  #define COLOR_BG              GColorBlack
  #define COLOR_STARS           GColorWhite
  #define COLOR_STARS_DIM       GColorWhite
  #define COLOR_SABER_RED       GColorWhite
  #define COLOR_SABER_RED_GLOW  GColorWhite
  #define COLOR_SABER_GREEN     GColorWhite
  #define COLOR_SABER_GREEN_GLOW GColorWhite
  #define COLOR_SABER_CORE      GColorWhite
  #define COLOR_HILT            GColorWhite
  #define COLOR_HILT_DARK       GColorWhite
  #define COLOR_VADER_CAPE      GColorWhite
  #define COLOR_VADER_OUTLINE   GColorWhite
  #define COLOR_LUKE_BODY       GColorWhite
  #define COLOR_LUKE_SKIN       GColorWhite
  #define COLOR_TIME_TEXT       GColorWhite
  #define COLOR_DATE_TEXT       GColorWhite
  #define COLOR_HOUR_MARKERS    GColorWhite
  #define COLOR_CENTER_DOT      GColorWhite
  #define COLOR_BATTERY_GOOD    GColorWhite
  #define COLOR_BATTERY_MED     GColorWhite
  #define COLOR_BATTERY_LOW     GColorWhite
#endif

// ============================================================================
// CONSTANTS
// ============================================================================

#define HOUR_BLADE_LEN   50
#define MINUTE_BLADE_LEN 70
#define HILT_LEN         11
#define CHAR_OFFSET      28
#define MARKER_RADIUS    88
#define NUM_STARS        30

// ============================================================================
// DATA STRUCTURES
// ============================================================================

typedef struct {
    int32_t angle;    // radial angle from screen center
    int16_t dist;     // distance from center
    uint8_t streak;   // streak length (2-6px)
    bool dim;
} Star;

// ============================================================================
// GLOBAL STATE
// ============================================================================

static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;

static GPoint s_center;
static int s_battery_level = 100;

static Star s_stars[NUM_STARS];

static char s_time_buf[8];
static char s_date_buf[16];
static char s_weather_buf[32];

// GPath objects for lightsaber blades and hilts
static GPath *s_hour_blade_path = NULL;
static GPath *s_min_blade_path = NULL;
static GPath *s_hour_hilt_path = NULL;
static GPath *s_min_hilt_path = NULL;

// Hour blade shape: narrow trapezoid extending upward
static GPoint s_hour_blade_pts[] = {
    {-3,  5}, { 3,  5}, { 2, -48}, {-2, -48}
};
static GPathInfo s_hour_blade_info = {
    .num_points = 4, .points = s_hour_blade_pts
};

// Minute blade shape: longer trapezoid
static GPoint s_min_blade_pts[] = {
    {-3,  5}, { 3,  5}, { 2, -68}, {-2, -68}
};
static GPathInfo s_min_blade_info = {
    .num_points = 4, .points = s_min_blade_pts
};

// Hour hilt
static GPoint s_hour_hilt_pts[] = {
    {-4,  5}, { 4,  5}, { 4, 16}, {-4, 16}
};
static GPathInfo s_hour_hilt_info = {
    .num_points = 4, .points = s_hour_hilt_pts
};

// Minute hilt
static GPoint s_min_hilt_pts[] = {
    {-4,  5}, { 4,  5}, { 4, 16}, {-4, 16}
};
static GPathInfo s_min_hilt_info = {
    .num_points = 4, .points = s_min_hilt_pts
};

// ============================================================================
// UTILITY
// ============================================================================

static int random_in_range(int min, int max) {
    if (max <= min) return min;
    return min + (rand() % (max - min + 1));
}

// ============================================================================
// STAR BACKGROUND
// ============================================================================

static void init_stars(int screen_w, int screen_h) {
    int max_dist = 140;
    for (int i = 0; i < NUM_STARS; i++) {
        // Place stars at random x,y positions, then convert to polar
        // This gives even spatial distribution (no center clustering)
        int16_t sx, sy;
        do {
            sx = random_in_range(0, screen_w);
            sy = random_in_range(0, screen_h);
        } while (sy < 32 || sy > screen_h - 28);  // avoid text zones

        int16_t dx = sx - screen_w / 2;
        int16_t dy = sy - screen_h / 2;
        s_stars[i].angle = atan2_lookup(dx, -dy);
        // Integer distance approximation (avoid float sqrt)
        int adx = abs(dx);
        int ady = abs(dy);
        s_stars[i].dist = (adx > ady) ? adx + ady / 2 : ady + adx / 2;

        // Longer streaks for stars farther from center
        s_stars[i].streak = (uint8_t)(12 + (s_stars[i].dist * 30) / max_dist);
        s_stars[i].dim = (random_in_range(0, 2) == 0);
    }
}

static void draw_stars(GContext *ctx) {
    graphics_context_set_antialiased(ctx, true);

    for (int i = 0; i < NUM_STARS; i++) {
        // Inner point (closer to center)
        int16_t x1 = s_center.x + (sin_lookup(s_stars[i].angle) * s_stars[i].dist) / TRIG_MAX_RATIO;
        int16_t y1 = s_center.y - (cos_lookup(s_stars[i].angle) * s_stars[i].dist) / TRIG_MAX_RATIO;

        // Outer point (streak extends outward from center)
        int outer_dist = s_stars[i].dist + s_stars[i].streak;
        int16_t x2 = s_center.x + (sin_lookup(s_stars[i].angle) * outer_dist) / TRIG_MAX_RATIO;
        int16_t y2 = s_center.y - (cos_lookup(s_stars[i].angle) * outer_dist) / TRIG_MAX_RATIO;

        graphics_context_set_stroke_color(ctx,
            s_stars[i].dim ? COLOR_STARS_DIM : COLOR_STARS);
        graphics_context_set_stroke_width(ctx, 1);
        graphics_draw_line(ctx, GPoint(x1, y1), GPoint(x2, y2));
    }
}

// ============================================================================
// HOUR MARKERS
// ============================================================================

static void draw_hour_markers(GContext *ctx) {
    graphics_context_set_stroke_color(ctx, COLOR_HOUR_MARKERS);
    graphics_context_set_stroke_width(ctx, 1);

    for (int i = 0; i < 12; i++) {
        int32_t angle = (i * TRIG_MAX_ANGLE) / 12;
        int inner_r = MARKER_RADIUS - 5;
        int outer_r = MARKER_RADIUS;

        // Longer ticks at 12, 3, 6, 9
        if (i % 3 == 0) {
            inner_r = MARKER_RADIUS - 10;
            graphics_context_set_stroke_width(ctx, 2);
        } else {
            graphics_context_set_stroke_width(ctx, 1);
        }

        GPoint inner = {
            s_center.x + (sin_lookup(angle) * inner_r) / TRIG_MAX_RATIO,
            s_center.y - (cos_lookup(angle) * inner_r) / TRIG_MAX_RATIO
        };
        GPoint outer = {
            s_center.x + (sin_lookup(angle) * outer_r) / TRIG_MAX_RATIO,
            s_center.y - (cos_lookup(angle) * outer_r) / TRIG_MAX_RATIO
        };

        graphics_draw_line(ctx, inner, outer);
    }
}

// ============================================================================
// LIGHTSABER DRAWING
// ============================================================================

static void draw_saber_glow(GContext *ctx, int32_t angle, int length, GColor glow_color) {
    int16_t tip_x = s_center.x + (sin_lookup(angle) * length) / TRIG_MAX_RATIO;
    int16_t tip_y = s_center.y - (cos_lookup(angle) * length) / TRIG_MAX_RATIO;

    graphics_context_set_stroke_color(ctx, glow_color);
    graphics_context_set_stroke_width(ctx, 7);
    graphics_context_set_antialiased(ctx, true);
    graphics_draw_line(ctx, s_center, GPoint(tip_x, tip_y));
}

static void draw_saber_blade(GContext *ctx, GPath *blade_path, int32_t angle,
                              GColor blade_color) {
    gpath_rotate_to(blade_path, angle);
    gpath_move_to(blade_path, s_center);

    // Blade fill
    graphics_context_set_fill_color(ctx, blade_color);
    gpath_draw_filled(ctx, blade_path);

    // White core line for glow effect
    int core_len = (blade_path == s_hour_blade_path) ? HOUR_BLADE_LEN - 5 : MINUTE_BLADE_LEN - 5;
    int16_t core_x = s_center.x + (sin_lookup(angle) * core_len) / TRIG_MAX_RATIO;
    int16_t core_y = s_center.y - (cos_lookup(angle) * core_len) / TRIG_MAX_RATIO;

    graphics_context_set_stroke_color(ctx, COLOR_SABER_CORE);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, s_center, GPoint(core_x, core_y));
}

static void draw_saber_hilt(GContext *ctx, GPath *hilt_path, int32_t angle) {
    gpath_rotate_to(hilt_path, angle);
    gpath_move_to(hilt_path, s_center);

    graphics_context_set_fill_color(ctx, COLOR_HILT);
    gpath_draw_filled(ctx, hilt_path);
    graphics_context_set_stroke_color(ctx, COLOR_HILT_DARK);
    gpath_draw_outline(ctx, hilt_path);

    // Crossguard: short perpendicular line at the blade/hilt junction
    int32_t perp = angle + TRIG_MAX_ANGLE / 4;
    int16_t cg1_x = s_center.x + (sin_lookup(angle) * 5) / TRIG_MAX_RATIO
                   + (sin_lookup(perp) * 6) / TRIG_MAX_RATIO;
    int16_t cg1_y = s_center.y - (cos_lookup(angle) * 5) / TRIG_MAX_RATIO
                   - (cos_lookup(perp) * 6) / TRIG_MAX_RATIO;
    int16_t cg2_x = s_center.x + (sin_lookup(angle) * 5) / TRIG_MAX_RATIO
                   - (sin_lookup(perp) * 6) / TRIG_MAX_RATIO;
    int16_t cg2_y = s_center.y - (cos_lookup(angle) * 5) / TRIG_MAX_RATIO
                   + (cos_lookup(perp) * 6) / TRIG_MAX_RATIO;

    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, GPoint(cg1_x, cg1_y), GPoint(cg2_x, cg2_y));
}

// ============================================================================
// CHARACTER SILHOUETTES
// ============================================================================

static void draw_vader(GContext *ctx, int16_t x, int16_t y) {
    // --- Cape (wide, flowing shape) ---
    GPoint cape[] = {
        {x, y - 14},       // Shoulder center
        {x - 18, y + 20},  // Left cape hem (wide)
        {x - 14, y + 22},  // Left inner fold
        {x, y + 18},       // Center bottom
        {x + 14, y + 22},  // Right inner fold
        {x + 18, y + 20}   // Right cape hem
    };
    GPathInfo cape_info = { .num_points = 6, .points = cape };
    GPath *cape_path = gpath_create(&cape_info);
    graphics_context_set_fill_color(ctx, COLOR_VADER_CAPE);
    gpath_draw_filled(ctx, cape_path);
    gpath_destroy(cape_path);

    // --- Body / armor torso ---
    GPoint body[] = {
        {x - 10, y - 14},  // Left shoulder
        {x + 10, y - 14},  // Right shoulder
        {x + 9, y + 2},    // Right waist
        {x + 11, y + 18},  // Right leg
        {x + 4, y + 18},   // Right inner leg
        {x, y + 8},        // Crotch
        {x - 4, y + 18},   // Left inner leg
        {x - 11, y + 18},  // Left leg
        {x - 9, y + 2}     // Left waist
    };
    GPathInfo body_info = { .num_points = 9, .points = body };
    GPath *body_path = gpath_create(&body_info);
    graphics_context_set_fill_color(ctx, COLOR_VADER_OUTLINE);
    gpath_draw_filled(ctx, body_path);
    gpath_destroy(body_path);

    // --- Belt ---
    graphics_context_set_fill_color(ctx, COLOR_HILT);
    graphics_fill_rect(ctx, GRect(x - 9, y, 18, 3), 0, GCornerNone);
    // Belt buckle
    graphics_context_set_fill_color(ctx, COLOR_SABER_CORE);
    graphics_fill_rect(ctx, GRect(x - 2, y, 4, 3), 0, GCornerNone);

    // --- Chest panel (iconic control box) ---
    #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(x - 4, y - 10, 8, 6), 1, GCornersAll);
    // Colored buttons on chest panel
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_draw_pixel(ctx, GPoint(x - 2, y - 8));
    graphics_context_set_stroke_color(ctx, GColorBlue);
    graphics_draw_pixel(ctx, GPoint(x, y - 8));
    graphics_context_set_stroke_color(ctx, GColorGreen);
    graphics_draw_pixel(ctx, GPoint(x + 2, y - 8));
    #endif

    // --- Helmet ---
    // Helmet dome (angular, wide at top)
    graphics_context_set_fill_color(ctx, COLOR_VADER_OUTLINE);
    GPoint helmet[] = {
        {x - 9, y - 17},   // Left wide base
        {x - 7, y - 26},   // Left dome
        {x, y - 29},       // Top peak
        {x + 7, y - 26},   // Right dome
        {x + 9, y - 17}    // Right wide base
    };
    GPathInfo helmet_info = { .num_points = 5, .points = helmet };
    GPath *helmet_path = gpath_create(&helmet_info);
    gpath_draw_filled(ctx, helmet_path);
    gpath_destroy(helmet_path);

    // Face plate (darker inset)
    graphics_context_set_fill_color(ctx, COLOR_BG);
    GPoint faceplate[] = {
        {x - 5, y - 17},
        {x - 4, y - 23},
        {x + 4, y - 23},
        {x + 5, y - 17}
    };
    GPathInfo face_info = { .num_points = 4, .points = faceplate };
    GPath *face_path = gpath_create(&face_info);
    gpath_draw_filled(ctx, face_path);
    gpath_destroy(face_path);

    // Triangular mouth grill
    graphics_context_set_stroke_color(ctx, COLOR_VADER_CAPE);
    graphics_context_set_stroke_width(ctx, 1);
    GPoint mouth[] = {
        {x - 3, y - 19},
        {x, y - 17},
        {x + 3, y - 19}
    };
    GPathInfo mouth_info = { .num_points = 3, .points = mouth };
    GPath *mouth_path = gpath_create(&mouth_info);
    gpath_draw_outline(ctx, mouth_path);
    gpath_destroy(mouth_path);

    // Eye lenses (menacing red)
    #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_rect(ctx, GRect(x - 4, y - 22, 3, 2), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(x + 1, y - 22, 3, 2), 0, GCornerNone);
    #else
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(x - 4, y - 22, 3, 2), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(x + 1, y - 22, 3, 2), 0, GCornerNone);
    #endif

    // Helmet outline for definition
    graphics_context_set_stroke_color(ctx, COLOR_VADER_OUTLINE);
    graphics_context_set_stroke_width(ctx, 1);
}

static void draw_luke(GContext *ctx, int16_t x, int16_t y) {
    // --- Tunic / body ---
    GPoint tunic[] = {
        {x - 8, y - 12},   // Left shoulder
        {x + 8, y - 12},   // Right shoulder
        {x + 7, y + 2},    // Right waist
        {x + 10, y + 18},  // Right leg
        {x + 3, y + 18},   // Right inner leg
        {x, y + 8},        // Center
        {x - 3, y + 18},   // Left inner leg
        {x - 10, y + 18},  // Left leg
        {x - 7, y + 2}     // Left waist
    };
    GPathInfo tunic_info = { .num_points = 9, .points = tunic };
    GPath *tunic_path = gpath_create(&tunic_info);
    graphics_context_set_fill_color(ctx, COLOR_LUKE_BODY);
    gpath_draw_filled(ctx, tunic_path);
    gpath_destroy(tunic_path);

    // --- Tunic overlap / vest (lighter center panel) ---
    #ifdef PBL_COLOR
    GPoint vest[] = {
        {x - 3, y - 12},
        {x + 3, y - 12},
        {x + 2, y + 4},
        {x - 2, y + 4}
    };
    GPathInfo vest_info = { .num_points = 4, .points = vest };
    GPath *vest_path = gpath_create(&vest_info);
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    gpath_draw_filled(ctx, vest_path);
    gpath_destroy(vest_path);
    #endif

    // --- Belt ---
    graphics_context_set_fill_color(ctx, COLOR_HILT);
    graphics_fill_rect(ctx, GRect(x - 7, y + 1, 14, 2), 0, GCornerNone);

    // --- Neck ---
    graphics_context_set_fill_color(ctx, COLOR_LUKE_SKIN);
    graphics_fill_rect(ctx, GRect(x - 2, y - 14, 4, 3), 0, GCornerNone);

    // --- Head ---
    graphics_context_set_fill_color(ctx, COLOR_LUKE_SKIN);
    graphics_fill_circle(ctx, GPoint(x, y - 19), 6);

    // --- Hair (blond, fuller) ---
    #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorRajah);
    #else
    graphics_context_set_fill_color(ctx, GColorWhite);
    #endif
    // Hair top
    graphics_fill_rect(ctx, GRect(x - 6, y - 26, 12, 6), 4, GCornersTop);
    // Side hair
    graphics_fill_rect(ctx, GRect(x - 7, y - 23, 3, 5), 1, GCornersLeft);
    graphics_fill_rect(ctx, GRect(x + 4, y - 23, 3, 5), 1, GCornersRight);

    // --- Eyes ---
    graphics_context_set_fill_color(ctx, COLOR_BG);
    graphics_fill_rect(ctx, GRect(x - 3, y - 20, 2, 2), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(x + 1, y - 20, 2, 2), 0, GCornerNone);

    // --- Mouth ---
    graphics_context_set_stroke_color(ctx, COLOR_BG);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(x - 2, y - 16), GPoint(x + 2, y - 16));
}

// ============================================================================
// BATTERY
// ============================================================================

static void draw_battery(GContext *ctx, GRect bounds) {
    int bar_w = 20;
    int bar_h = 6;
    int bar_x = bounds.size.w - bar_w - 5;
    int bar_y = 5;

    // Outline
    graphics_context_set_stroke_color(ctx, COLOR_HOUR_MARKERS);
    graphics_draw_rect(ctx, GRect(bar_x, bar_y, bar_w, bar_h));

    // Fill
    int fill_w = (s_battery_level * bar_w) / 100;
    #ifdef PBL_COLOR
    GColor fill_color = COLOR_BATTERY_GOOD;
    if (s_battery_level <= 20) fill_color = COLOR_BATTERY_LOW;
    else if (s_battery_level <= 40) fill_color = COLOR_BATTERY_MED;
    graphics_context_set_fill_color(ctx, fill_color);
    #else
    graphics_context_set_fill_color(ctx, GColorWhite);
    #endif
    graphics_fill_rect(ctx, GRect(bar_x, bar_y, fill_w, bar_h), 0, GCornerNone);

    // Tip
    graphics_fill_rect(ctx, GRect(bar_x + bar_w, bar_y + 1, 2, bar_h - 2), 0, GCornerNone);
}

// ============================================================================
// MAIN CANVAS UPDATE
// ============================================================================

static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    // 1. Clear background
    graphics_context_set_fill_color(ctx, COLOR_BG);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // 2. Draw stars
    draw_stars(ctx);

    // 3. Draw hour markers
    draw_hour_markers(ctx);

    // Get current time for hand angles
    time_t temp = time(NULL);
    struct tm *t = localtime(&temp);
    if (!t) return;

    // Hour angle (smooth interpolation with minutes)
    int32_t hour_angle = ((t->tm_hour % 12) * TRIG_MAX_ANGLE / 12) +
                         (t->tm_min * TRIG_MAX_ANGLE / 12 / 60);
    // Minute angle
    int32_t min_angle = (t->tm_min * TRIG_MAX_ANGLE / 60);

    // 4. Calculate character positions (at base of their saber, opposite blade direction)
    int32_t vader_dir = hour_angle + TRIG_MAX_ANGLE / 2;
    int16_t vader_x = s_center.x + (sin_lookup(vader_dir) * CHAR_OFFSET) / TRIG_MAX_RATIO;
    int16_t vader_y = s_center.y - (cos_lookup(vader_dir) * CHAR_OFFSET) / TRIG_MAX_RATIO;

    int32_t luke_dir = min_angle + TRIG_MAX_ANGLE / 2;
    int16_t luke_x = s_center.x + (sin_lookup(luke_dir) * CHAR_OFFSET) / TRIG_MAX_RATIO;
    int16_t luke_y = s_center.y - (cos_lookup(luke_dir) * CHAR_OFFSET) / TRIG_MAX_RATIO;

    // 5. Draw character silhouettes BEHIND sabers
    draw_vader(ctx, vader_x, vader_y);
    draw_luke(ctx, luke_x, luke_y);

    // 6. Draw saber glows
    draw_saber_glow(ctx, hour_angle, HOUR_BLADE_LEN, COLOR_SABER_RED_GLOW);
    draw_saber_glow(ctx, min_angle, MINUTE_BLADE_LEN, COLOR_SABER_GREEN_GLOW);

    // 7. Draw lightsaber blades (on top of characters)
    draw_saber_blade(ctx, s_hour_blade_path, hour_angle, COLOR_SABER_RED);
    draw_saber_blade(ctx, s_min_blade_path, min_angle, COLOR_SABER_GREEN);

    // 8. Draw lightsaber hilts
    draw_saber_hilt(ctx, s_hour_hilt_path, hour_angle);
    draw_saber_hilt(ctx, s_min_hilt_path, min_angle);

    // 9. Center clash dot (the duel point)
    graphics_context_set_fill_color(ctx, COLOR_CENTER_DOT);
    graphics_fill_circle(ctx, s_center, 3);
    graphics_context_set_fill_color(ctx, COLOR_SABER_CORE);
    graphics_fill_circle(ctx, s_center, 1);

    // 10. Battery
    draw_battery(ctx, bounds);
}

// ============================================================================
// WEATHER (AppMessage)
// ============================================================================

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
    Tuple *cond_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

    if (temp_tuple && cond_tuple) {
        snprintf(s_weather_buf, sizeof(s_weather_buf),
                 "%s %d\u00b0", cond_tuple->value->cstring, (int)temp_tuple->value->int32);
        if (s_weather_layer) {
            text_layer_set_text(s_weather_layer, s_weather_buf);
        }
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator,
                                    AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// ============================================================================
// TIME HANDLING
// ============================================================================

static void update_time(void) {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    if (!tick_time) return;

    strftime(s_time_buf, sizeof(s_time_buf),
             clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    text_layer_set_text(s_time_layer, s_time_buf);

    strftime(s_date_buf, sizeof(s_date_buf), "%a, %b %d", tick_time);
    text_layer_set_text(s_date_layer, s_date_buf);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
    if (s_canvas_layer) {
        layer_mark_dirty(s_canvas_layer);
    }

    // Request weather update every 30 minutes
    if (tick_time->tm_min % 30 == 0) {
        DictionaryIterator *iter;
        AppMessageResult result = app_message_outbox_begin(&iter);
        if (result == APP_MSG_OK) {
            dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
            app_message_outbox_send();
        }
    }
}

// ============================================================================
// BATTERY
// ============================================================================

static void battery_callback(BatteryChargeState state) {
    s_battery_level = state.charge_percent;
    if (s_canvas_layer) {
        layer_mark_dirty(s_canvas_layer);
    }
}

// ============================================================================
// WINDOW HANDLERS
// ============================================================================

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_center = GPoint(bounds.size.w / 2, bounds.size.h / 2);

    // Initialize stars
    init_stars(bounds.size.w, bounds.size.h);

    // Create GPath objects
    s_hour_blade_path = gpath_create(&s_hour_blade_info);
    s_min_blade_path = gpath_create(&s_min_blade_info);
    s_hour_hilt_path = gpath_create(&s_hour_hilt_info);
    s_min_hilt_path = gpath_create(&s_min_hilt_info);

    // Canvas layer (full screen)
    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);

    // Digital time at top (Star Wars yellow)
    s_time_layer = text_layer_create(GRect(0, 2, bounds.size.w, 30));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, COLOR_TIME_TEXT);
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

    // Date at bottom
    s_date_layer = text_layer_create(GRect(0, bounds.size.h - 40, bounds.size.w, 20));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, COLOR_DATE_TEXT);
    text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    // Weather (Star Wars planet) below date
    s_weather_layer = text_layer_create(GRect(0, bounds.size.h - 22, bounds.size.w, 20));
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, COLOR_TIME_TEXT);
    text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
    text_layer_set_text(s_weather_layer, "Scanning...");
    layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));

    // Initial update
    update_time();
}

static void main_window_unload(Window *window) {
    if (s_hour_blade_path) { gpath_destroy(s_hour_blade_path); s_hour_blade_path = NULL; }
    if (s_min_blade_path)  { gpath_destroy(s_min_blade_path);  s_min_blade_path = NULL; }
    if (s_hour_hilt_path)  { gpath_destroy(s_hour_hilt_path);  s_hour_hilt_path = NULL; }
    if (s_min_hilt_path)   { gpath_destroy(s_min_hilt_path);   s_min_hilt_path = NULL; }

    if (s_canvas_layer)   { layer_destroy(s_canvas_layer); s_canvas_layer = NULL; }
    if (s_time_layer)     { text_layer_destroy(s_time_layer); s_time_layer = NULL; }
    if (s_date_layer)     { text_layer_destroy(s_date_layer); s_date_layer = NULL; }
    if (s_weather_layer)  { text_layer_destroy(s_weather_layer); s_weather_layer = NULL; }
}

// ============================================================================
// APPLICATION LIFECYCLE
// ============================================================================

static void init(void) {
    srand(time(NULL));

    s_main_window = window_create();
    window_set_background_color(s_main_window, COLOR_BG);
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    window_stack_push(s_main_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_callback);
    s_battery_level = battery_state_service_peek().charge_percent;

    // AppMessage for weather
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    app_message_open(128, 128);
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
