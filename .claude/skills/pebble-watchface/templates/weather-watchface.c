/**
 * Weather Pebble Watchface Template
 *
 * Displays time, date, and weather information fetched via
 * PebbleKit JS from the Open-Meteo API. Requires a companion
 * src/pkjs/index.js file for phone-side communication.
 *
 * Battery-efficient: updates on MINUTE_UNIT only.
 * Weather refreshes every 30 minutes.
 */

#include <pebble.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static Layer *s_battery_layer;

static int s_battery_level = 100;

// ============================================================================
// TIME HANDLING
// ============================================================================

static void update_time(void) {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    if (!tick_time) return;

    static char time_buffer[8];
    strftime(time_buffer, sizeof(time_buffer),
             clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    text_layer_set_text(s_time_layer, time_buffer);

    static char date_buffer[24];
    strftime(date_buffer, sizeof(date_buffer), "%a, %b %d", tick_time);
    text_layer_set_text(s_date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();

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
// WEATHER HANDLING (AppMessage)
// ============================================================================

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
    Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

    if (temp_tuple && conditions_tuple) {
        static char temperature_buffer[8];
        static char conditions_buffer[32];
        static char weather_layer_buffer[42];

        snprintf(temperature_buffer, sizeof(temperature_buffer),
                 "%d\u00b0C", (int)temp_tuple->value->int32);
        snprintf(conditions_buffer, sizeof(conditions_buffer),
                 "%s", conditions_tuple->value->cstring);
        snprintf(weather_layer_buffer, sizeof(weather_layer_buffer),
                 "%s %s", temperature_buffer, conditions_buffer);

        text_layer_set_text(s_weather_layer, weather_layer_buffer);
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
// BATTERY HANDLING
// ============================================================================

static void battery_callback(BatteryChargeState state) {
    s_battery_level = state.charge_percent;
    if (s_battery_layer) layer_mark_dirty(s_battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int bar_width = (s_battery_level * bounds.size.w) / 100;

    #ifdef PBL_COLOR
    if (s_battery_level <= 20) {
        graphics_context_set_fill_color(ctx, GColorRed);
    } else if (s_battery_level <= 40) {
        graphics_context_set_fill_color(ctx, GColorYellow);
    } else {
        graphics_context_set_fill_color(ctx, GColorGreen);
    }
    #else
    graphics_context_set_fill_color(ctx, GColorWhite);
    #endif

    graphics_fill_rect(ctx, GRect(0, 0, bar_width, bounds.size.h), 0, GCornerNone);
}

// ============================================================================
// WINDOW HANDLERS
// ============================================================================

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Battery bar across top
    s_battery_layer = layer_create(GRect(0, 0, bounds.size.w, 3));
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    layer_add_child(window_layer, s_battery_layer);

    // Time layer - large centered text
    int time_y = PBL_IF_ROUND_ELSE(bounds.size.h / 2 - 50, bounds.size.h / 2 - 55);
    s_time_layer = text_layer_create(GRect(0, time_y, bounds.size.w, 50));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

    // Date layer
    int date_y = time_y + 52;
    s_date_layer = text_layer_create(GRect(0, date_y, bounds.size.w, 26));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorWhite);
    text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    // Weather layer
    int weather_y = date_y + 30;
    s_weather_layer = text_layer_create(GRect(0, weather_y, bounds.size.w, 24));
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, GColorWhite);
    text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
    text_layer_set_text(s_weather_layer, "Loading...");
    layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));

    update_time();
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    text_layer_destroy(s_weather_layer);
    layer_destroy(s_battery_layer);
}

// ============================================================================
// APPLICATION LIFECYCLE
// ============================================================================

static void init(void) {
    s_main_window = window_create();
    window_set_background_color(s_main_window, GColorBlack);
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    window_stack_push(s_main_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_callback);
    battery_callback(battery_state_service_peek());

    // Register AppMessage callbacks BEFORE opening
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    app_message_open(128, 128);
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
