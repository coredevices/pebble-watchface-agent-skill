# Pebble API Reference

## Core Header
```c
#include <pebble.h>
```

## Data Types

### Points and Rectangles
```c
typedef struct {
    int16_t x;
    int16_t y;
} GPoint;

typedef struct {
    GPoint origin;
    GSize size;
} GRect;

typedef struct {
    int16_t w;
    int16_t h;
} GSize;
```

### Colors
```c
// Basic colors (work on all platforms)
GColorBlack
GColorWhite
GColorClear  // Transparent

// Color display colors (basalt, chalk, emery, gabbro, flint)
GColorRed, GColorGreen, GColorBlue
GColorYellow, GColorCyan, GColorMagenta
GColorOrange, GColorPurple, GColorPink
// ... and many more (64 colors total)

// Create custom color
GColorFromRGB(r, g, b)  // r, g, b: 0-255
GColorFromHEX(0xRRGGBB)
```

## Graphics Context

### Setting Colors
```c
graphics_context_set_fill_color(GContext *ctx, GColor color);
graphics_context_set_stroke_color(GContext *ctx, GColor color);
graphics_context_set_stroke_width(GContext *ctx, uint8_t width);  // odd values only
graphics_context_set_text_color(GContext *ctx, GColor color);
graphics_context_set_antialiased(GContext *ctx, bool enable);  // default true
```

### Drawing Shapes
```c
// Circles
graphics_fill_circle(GContext *ctx, GPoint center, uint16_t radius);
graphics_draw_circle(GContext *ctx, GPoint center, uint16_t radius);

// Rectangles
graphics_fill_rect(GContext *ctx, GRect rect, uint16_t corner_radius, GCornerMask corners);
graphics_draw_rect(GContext *ctx, GRect rect);
graphics_draw_round_rect(GContext *ctx, GRect rect, uint16_t radius);

// Lines
graphics_draw_line(GContext *ctx, GPoint start, GPoint end);

// Pixels
graphics_draw_pixel(GContext *ctx, GPoint point);

// Arcs and radial fills
graphics_draw_arc(GContext *ctx, GRect rect, GOvalScaleMode scale_mode,
                  int32_t angle_start, int32_t angle_end);
graphics_fill_radial(GContext *ctx, GRect rect, GOvalScaleMode scale_mode,
                     uint16_t inset_thickness, int32_t angle_start, int32_t angle_end);

// Polar coordinate helpers
GPoint gpoint_from_polar(GRect rect, GOvalScaleMode scale_mode, int32_t angle);
GRect grect_centered_from_polar(GRect rect, GOvalScaleMode scale_mode,
                                 int32_t angle, GSize size);
```

### Oval Scale Modes
```c
GOvalScaleModeFitCircle    // inscribed circle
GOvalScaleModeFillCircle   // circumscribed circle
```

### Corner Masks for Rectangles
```c
GCornerNone
GCornersAll
GCornersTop
GCornersBottom
GCornersLeft
GCornersRight
GCornerTopLeft
GCornerTopRight
GCornerBottomLeft
GCornerBottomRight
```

## Paths (Vector Graphics)

### Creating Paths
```c
static const GPathInfo PATH_INFO = {
    .num_points = 3,
    .points = (GPoint[]) {
        {0, 0}, {10, 20}, {20, 0}
    }
};

// Create path (do this once in window_load, not in update_proc!)
GPath *path = gpath_create(&PATH_INFO);
```

### Drawing Paths
```c
gpath_draw_filled(GContext *ctx, GPath *path);
gpath_draw_outline(GContext *ctx, GPath *path);
```

### Transforming Paths
```c
gpath_move_to(GPath *path, GPoint point);
gpath_rotate_to(GPath *path, int32_t angle);  // angle in TRIG_MAX_ANGLE units
```

### Destroying Paths
```c
gpath_destroy(GPath *path);  // Call in window_unload!
```

## Trigonometry (Fixed-Point)

**IMPORTANT**: Pebble uses fixed-point math. No floating point!

### Constants
```c
TRIG_MAX_ANGLE  // Full circle = 65536 (0x10000)
TRIG_MAX_RATIO  // Maximum sin/cos value = 65536

// Conversion macros
DEG_TO_TRIGANGLE(degrees)  // Convert degrees to trig angle
```

### Functions
```c
int32_t sin_lookup(int32_t angle);   // [-TRIG_MAX_RATIO, TRIG_MAX_RATIO]
int32_t cos_lookup(int32_t angle);
int32_t atan2_lookup(int16_t y, int16_t x);
```

### Usage Example
```c
int32_t angle = (hour * TRIG_MAX_ANGLE) / 12;
int16_t x = center.x + (sin_lookup(angle) * radius) / TRIG_MAX_RATIO;
int16_t y = center.y - (cos_lookup(angle) * radius) / TRIG_MAX_RATIO;
```

## Layers

### Window Layer
```c
Layer *window_get_root_layer(Window *window);
GRect layer_get_bounds(Layer *layer);
GRect layer_get_unobstructed_bounds(Layer *layer);  // excludes timeline peek
```

### Custom Layers
```c
Layer *layer_create(GRect frame);
void layer_destroy(Layer *layer);
void layer_set_update_proc(Layer *layer, LayerUpdateProc update_proc);
void layer_add_child(Layer *parent, Layer *child);
void layer_mark_dirty(Layer *layer);  // Request redraw
void layer_set_hidden(Layer *layer, bool hidden);
void layer_set_frame(Layer *layer, GRect frame);
```

### Update Procedure
```c
static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    // Draw here using bounds.size.w and bounds.size.h
}
```

### Text Layers
```c
TextLayer *text_layer_create(GRect frame);
void text_layer_destroy(TextLayer *layer);
void text_layer_set_text(TextLayer *layer, const char *text);
void text_layer_set_font(TextLayer *layer, GFont font);
void text_layer_set_text_color(TextLayer *layer, GColor color);
void text_layer_set_background_color(TextLayer *layer, GColor color);
void text_layer_set_text_alignment(TextLayer *layer, GTextAlignment alignment);
void text_layer_set_overflow_mode(TextLayer *layer, GTextOverflowMode mode);
Layer *text_layer_get_layer(TextLayer *layer);
```

### Text Alignment & Overflow
```c
GTextAlignmentLeft
GTextAlignmentCenter
GTextAlignmentRight

GTextOverflowModeWordWrap
GTextOverflowModeTrailingEllipsis
GTextOverflowModeFill
```

### Drawing Text Directly
```c
graphics_draw_text(GContext *ctx, const char *text, GFont font,
                   GRect box, GTextOverflowMode overflow,
                   GTextAlignment alignment, GTextAttributes *attributes);
GSize graphics_text_layout_get_content_size(const char *text, GFont font,
                   GRect box, GTextOverflowMode overflow,
                   GTextAlignment alignment);
```

## Fonts

### System Fonts
```c
GFont fonts_get_system_font(const char *font_key);

// Common font keys
FONT_KEY_GOTHIC_14
FONT_KEY_GOTHIC_14_BOLD
FONT_KEY_GOTHIC_18
FONT_KEY_GOTHIC_18_BOLD
FONT_KEY_GOTHIC_24
FONT_KEY_GOTHIC_24_BOLD
FONT_KEY_GOTHIC_28
FONT_KEY_GOTHIC_28_BOLD
FONT_KEY_BITHAM_30_BLACK
FONT_KEY_BITHAM_42_BOLD
FONT_KEY_BITHAM_42_LIGHT
FONT_KEY_ROBOTO_CONDENSED_21
FONT_KEY_LECO_20_BOLD_NUMBERS
FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM
FONT_KEY_LECO_32_BOLD_NUMBERS
FONT_KEY_LECO_36_BOLD_NUMBERS
FONT_KEY_LECO_38_BOLD_NUMBERS
FONT_KEY_LECO_42_NUMBERS
```

### Custom Fonts
```c
GFont fonts_load_custom_font(ResHandle handle);
void fonts_unload_custom_font(GFont font);
// Usage: fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MY_FONT_42))
```

## Windows

### Creating Windows
```c
Window *window_create(void);
void window_destroy(Window *window);
void window_set_background_color(Window *window, GColor color);
void window_set_window_handlers(Window *window, WindowHandlers handlers);
void window_stack_push(Window *window, bool animated);
```

### Window Handlers
```c
window_set_window_handlers(window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
});
```

## Time

### Getting Current Time
```c
time_t temp = time(NULL);
struct tm *tick_time = localtime(&temp);

tick_time->tm_hour   // 0-23
tick_time->tm_min    // 0-59
tick_time->tm_sec    // 0-59
tick_time->tm_mday   // 1-31
tick_time->tm_mon    // 0-11
tick_time->tm_year   // Years since 1900
tick_time->tm_wday   // 0-6 (Sunday = 0)
```

### Time Formatting
```c
static char buffer[8];
strftime(buffer, sizeof(buffer), "%H:%M", tick_time);  // 24-hour
strftime(buffer, sizeof(buffer), "%I:%M", tick_time);  // 12-hour

// Check user preference
clock_is_24h_style()  // returns bool
```

### Tick Timer Service
```c
void tick_timer_service_subscribe(TimeUnits units, TickHandler handler);
void tick_timer_service_unsubscribe(void);

// TimeUnits — ALWAYS USE MINUTE_UNIT for watchfaces (battery efficiency!)
MINUTE_UNIT   // ← DEFAULT for watchfaces
HOUR_UNIT
DAY_UNIT
SECOND_UNIT   // ← AVOID: drains battery rapidly

// Handler
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
}
```

**WARNING**: Using `SECOND_UNIT` causes a redraw every second and significantly reduces battery life. Only use if the user explicitly requests seconds display.

## App Timers (for Animation)

```c
AppTimer *app_timer_register(uint32_t timeout_ms, AppTimerCallback callback, void *data);
void app_timer_cancel(AppTimer *timer);
bool app_timer_reschedule(AppTimer *timer, uint32_t new_timeout_ms);

static void timer_callback(void *data) {
    layer_mark_dirty(s_canvas_layer);
    s_timer = app_timer_register(50, timer_callback, NULL);  // 50ms = 20 FPS
}
```

## Battery Service

```c
BatteryChargeState battery_state_service_peek(void);
void battery_state_service_subscribe(BatteryStateHandler handler);
void battery_state_service_unsubscribe(void);

typedef struct {
    uint8_t charge_percent;  // 0-100
    bool is_charging;
    bool is_plugged;
} BatteryChargeState;
```

## Connection Service

```c
bool connection_service_peek_pebble_app_connection(void);
void connection_service_subscribe(ConnectionHandlers handlers);
void connection_service_unsubscribe(void);

connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
});
// Callback: void(bool connected)
```

## Vibration

```c
vibes_short_pulse(void);
vibes_long_pulse(void);
vibes_double_pulse(void);
```

## AppMessage (Phone ↔ Watch Communication)

Used for weather, web data, and configuration. Requires PebbleKit JS on the phone side.

### Setup (C side)
```c
// Register callbacks BEFORE opening (in init())
app_message_register_inbox_received(inbox_received_callback);
app_message_register_inbox_dropped(inbox_dropped_callback);
app_message_register_outbox_failed(outbox_failed_callback);
app_message_register_outbox_sent(outbox_sent_callback);

// Open with buffer sizes
app_message_open(128, 128);  // inbox_size, outbox_size
```

### Receiving Messages (C side)
```c
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Look up tuples by MESSAGE_KEY_* (auto-generated from package.json messageKeys)
    Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
    Tuple *cond_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

    if (temp_tuple) {
        int temperature = (int)temp_tuple->value->int32;
    }
    if (cond_tuple) {
        char *conditions = cond_tuple->value->cstring;
    }
}
```

### Sending Messages (C side)
```c
DictionaryIterator *iter;
app_message_outbox_begin(&iter);
dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
app_message_outbox_send();
```

### PebbleKit JS (Phone side — src/pkjs/index.js)
```javascript
// Send data to watch
Pebble.sendAppMessage(
    { 'TEMPERATURE': 72, 'CONDITIONS': 'Clear' },
    function(e) { console.log('Sent!'); },
    function(e) { console.log('Failed!'); }
);

// Receive from watch
Pebble.addEventListener('appmessage', function(e) {
    if (e.payload['REQUEST_WEATHER']) {
        // Fetch weather and send back...
    }
});

// JS runtime ready
Pebble.addEventListener('ready', function(e) {
    // Safe to start fetching data
});

// Available APIs in PebbleKit JS:
// - XMLHttpRequest (HTTP requests)
// - navigator.geolocation (GPS)
// - localStorage (persistent key-value storage)
// - Pebble.getActiveWatchInfo() (watch platform, model, firmware)
```

### package.json Requirements for AppMessage
```json
{
  "pebble": {
    "enableMultiJS": true,
    "capabilities": ["location"],
    "messageKeys": ["TEMPERATURE", "CONDITIONS", "REQUEST_WEATHER"]
  }
}
```

Message keys become `MESSAGE_KEY_*` constants in C code automatically.

## Unobstructed Area (Quick View)

Handle timeline peek that covers part of the screen:
```c
UnobstructedAreaHandlers handlers = {
    .will_change = prv_unobstructed_will_change,
    .change = prv_unobstructed_change,
    .did_change = prv_unobstructed_did_change
};
unobstructed_area_service_subscribe(handlers, NULL);

// Get visible bounds (excluding peek area)
GRect visible = layer_get_unobstructed_bounds(window_layer);
```

## Random Numbers

```c
#include <stdlib.h>
srand(time(NULL));  // Seed once in init()
int value = rand() % range;
```

## Logging

```c
APP_LOG(APP_LOG_LEVEL_DEBUG, "Debug: %d", value);
APP_LOG(APP_LOG_LEVEL_INFO, "Info message");
APP_LOG(APP_LOG_LEVEL_WARNING, "Warning!");
APP_LOG(APP_LOG_LEVEL_ERROR, "Error!");
```

## Platform Detection

```c
#ifdef PBL_COLOR
    // Color display (basalt, chalk, emery, gabbro, flint)
#else
    // Black and white (aplite, diorite)
#endif

#ifdef PBL_ROUND
    // Round display (chalk, gabbro)
#else
    // Rectangular display (aplite, basalt, diorite, emery, flint)
#endif

// Round/rect conditional macro
PBL_IF_ROUND_ELSE(round_value, rect_value)
PBL_IF_COLOR_ELSE(color_value, bw_value)

// Platform-specific
#ifdef PBL_PLATFORM_EMERY    // Pebble Time 2 (200x228)
#endif
#ifdef PBL_PLATFORM_BASALT   // Pebble Time (144x168)
#endif
#ifdef PBL_PLATFORM_CHALK    // Pebble Time Round (180x180)
#endif
#ifdef PBL_PLATFORM_APLITE   // Pebble Classic (144x168, B&W)
#endif
#ifdef PBL_PLATFORM_DIORITE  // Pebble 2 (144x168, B&W)
#endif
```

## Application Lifecycle

```c
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
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
    return 0;
}
```

## Watch Info

```c
WatchInfoModel watch_info_get_model(void);
// Returns: PEBBLE_ORIGINAL, PEBBLE_TIME, PEBBLE_TIME_ROUND_14,
//          PEBBLE_2_HR, COREDEVICES_PT2, COREDEVICES_PR2, COREDEVICES_P2D

WatchInfoVersion watch_info_get_firmware_version(void);
WatchInfoColor watch_info_get_color(void);
```
