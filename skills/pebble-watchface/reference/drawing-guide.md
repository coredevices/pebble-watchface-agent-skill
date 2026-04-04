# Pebble Drawing Guide

## Screen Coordinates

```
(0,0) ────────────────────→ X
  │
  │      ┌─────────────┐
  │      │             │
  │      │   Screen    │
  │      │             │
  │      │             │
  │      └─────────────┘
  ↓
  Y
```

### Platform Dimensions
| Platform | Width | Height | Shape |
|----------|-------|--------|-------|
| emery    | 200   | 228    | Rect  |
| gabbro   | 260   | 260    | Round |
| basalt   | 144   | 168    | Rect  |
| chalk    | 180   | 180    | Round |
| aplite   | 144   | 168    | Rect  |
| diorite  | 144   | 168    | Rect  |
| flint    | 144   | 168    | Rect  |

**Always use `layer_get_bounds()` to get screen dimensions dynamically.** Do not hardcode pixel values.

```c
Layer *window_layer = window_get_root_layer(window);
GRect bounds = layer_get_bounds(window_layer);
int width = bounds.size.w;   // 200 on emery, 260 on gabbro, etc.
int height = bounds.size.h;  // 228 on emery, 260 on gabbro, etc.
```

## Basic Shapes

### Circles
```c
// Filled circle
graphics_context_set_fill_color(ctx, GColorWhite);
graphics_fill_circle(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2), 20);

// Outlined circle
graphics_context_set_stroke_color(ctx, GColorWhite);
graphics_draw_circle(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2), 20);
```

### Rectangles
```c
GRect rect = GRect(10, 10, 50, 30);  // x, y, width, height

// Filled rectangle
graphics_fill_rect(ctx, rect, 0, GCornerNone);  // Sharp corners

// Rounded rectangle
graphics_fill_rect(ctx, rect, 5, GCornersAll);  // 5px corner radius

// Only round specific corners
graphics_fill_rect(ctx, rect, 5, GCornersTop);
```

### Lines
```c
graphics_context_set_stroke_color(ctx, GColorWhite);
graphics_context_set_stroke_width(ctx, 2);
graphics_draw_line(ctx, GPoint(10, 10), GPoint(100, 100));
```

### Arcs and Radial Fills
```c
// Draw an arc (ring outline)
graphics_draw_arc(ctx, bounds, GOvalScaleModeFitCircle,
                  DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(270));

// Fill a radial segment (pie slice / ring segment)
graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle,
                     10,  // inset thickness
                     DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(270));

// Get a point on the circle perimeter
GPoint p = gpoint_from_polar(bounds, GOvalScaleModeFitCircle,
                              DEG_TO_TRIGANGLE(45));
```

## Paths (Complex Shapes)

### Triangle
```c
static GPoint triangle_points[3];
static GPathInfo triangle_info = {
    .num_points = 3,
    .points = triangle_points
};
static GPath *triangle = NULL;

// In window_load - set points relative to screen
triangle_points[0] = GPoint(bounds.size.w / 2, 20);    // Top center
triangle_points[1] = GPoint(20, bounds.size.h - 20);    // Bottom left
triangle_points[2] = GPoint(bounds.size.w - 20, bounds.size.h - 20);  // Bottom right
triangle = gpath_create(&triangle_info);

// In update_proc
graphics_context_set_fill_color(ctx, GColorWhite);
gpath_draw_filled(ctx, triangle);

// In window_unload
gpath_destroy(triangle);
```

### Star
```c
static GPoint star_points[10];

static void init_star(GPoint center, int outer_r, int inner_r) {
    for (int i = 0; i < 10; i++) {
        int32_t angle = (i * TRIG_MAX_ANGLE / 10) - (TRIG_MAX_ANGLE / 4);
        int radius = (i % 2 == 0) ? outer_r : inner_r;
        star_points[i].x = center.x + (sin_lookup(angle) * radius) / TRIG_MAX_RATIO;
        star_points[i].y = center.y + (cos_lookup(angle) * radius) / TRIG_MAX_RATIO;
    }
}
```

## Text Rendering

### Using Text Layers
```c
static TextLayer *s_time_layer;

// Create — use bounds for positioning
s_time_layer = text_layer_create(GRect(0, 50, bounds.size.w, 40));
text_layer_set_background_color(s_time_layer, GColorClear);
text_layer_set_text_color(s_time_layer, GColorWhite);
text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
```

### Drawing Text Directly
```c
static void draw_text(GContext *ctx, const char *text, GRect text_bounds) {
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, text,
                       fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       text_bounds,
                       GTextOverflowModeWordWrap,
                       GTextAlignmentCenter,
                       NULL);
}
```

## Color Management

### Platform-Aware Colors
```c
#ifdef PBL_COLOR
    #define BACKGROUND_COLOR GColorDarkGray
    #define FOREGROUND_COLOR GColorCyan
    #define ACCENT_COLOR GColorRed
#else
    #define BACKGROUND_COLOR GColorBlack
    #define FOREGROUND_COLOR GColorWhite
    #define ACCENT_COLOR GColorWhite
#endif
```

### Color Palette (64 colors on color platforms)
```c
// Primary
GColorRed, GColorGreen, GColorBlue

// Warm
GColorOrange, GColorYellow, GColorRajah, GColorMelon

// Cool
GColorCyan, GColorTiffanyBlue, GColorCadetBlue, GColorPictonBlue

// Purple/Pink
GColorMagenta, GColorPurple, GColorVividViolet, GColorShockingPink

// Neutrals
GColorBlack, GColorOxfordBlue, GColorDarkGray, GColorLightGray, GColorWhite
```

## Drawing Order (Painter's Algorithm)

Draw from back to front:

```c
static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    // 1. Clear background
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // 2. Draw background elements
    draw_background(ctx, bounds);

    // 3. Draw middle-ground elements
    draw_scenery(ctx, bounds);

    // 4. Draw foreground elements
    draw_characters(ctx, bounds);

    // 5. Draw UI overlays last
    draw_ui(ctx, bounds);
}
```

## Common Drawing Patterns

### Battery Bar
```c
static void draw_battery_bar(GContext *ctx, GRect bar_bounds, int percent) {
    // Outline
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_rect(ctx, bar_bounds);

    // Fill
    int fill_width = (bar_bounds.size.w * percent) / 100;
    GRect fill = {bar_bounds.origin, {fill_width, bar_bounds.size.h}};
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, fill, 0, GCornerNone);

    // Battery tip
    GRect tip = {
        {bar_bounds.origin.x + bar_bounds.size.w, bar_bounds.origin.y + 2},
        {2, bar_bounds.size.h - 4}
    };
    graphics_fill_rect(ctx, tip, 0, GCornerNone);
}
```

### Analog Clock Face
```c
static void draw_clock_face(GContext *ctx, GPoint center, int radius) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, center, radius);

    // Hour markers
    for (int i = 0; i < 12; i++) {
        int32_t angle = (i * TRIG_MAX_ANGLE) / 12;
        int inner_r = radius - 8;
        int outer_r = radius - 2;

        GPoint inner = {
            center.x + (sin_lookup(angle) * inner_r) / TRIG_MAX_RATIO,
            center.y - (cos_lookup(angle) * inner_r) / TRIG_MAX_RATIO
        };
        GPoint outer = {
            center.x + (sin_lookup(angle) * outer_r) / TRIG_MAX_RATIO,
            center.y - (cos_lookup(angle) * outer_r) / TRIG_MAX_RATIO
        };
        graphics_draw_line(ctx, inner, outer);
    }
}
```

## Performance Tips

1. **Use `layer_get_bounds()`** — Get dimensions dynamically, never hardcode
2. **Minimize draw calls** — Batch similar operations
3. **Pre-calculate positions** — Don't do math in draw functions if avoidable
4. **Use `layer_mark_dirty()`** — Only redraw when necessary
5. **Clip to visible area** — Skip drawing objects outside screen bounds

```c
// Check if point is on screen before drawing (uses dynamic bounds)
static bool is_visible(GPoint p, GRect bounds, int margin) {
    return p.x >= -margin && p.x <= bounds.size.w + margin &&
           p.y >= -margin && p.y <= bounds.size.h + margin;
}
```
