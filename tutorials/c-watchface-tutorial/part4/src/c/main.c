#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static Layer *s_battery_layer;
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;
static GFont s_time_font;
static GFont s_date_font;

static int s_battery_level;

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ?
                                                    "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %d", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  // Request weather update every 30 minutes
  if (tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
    app_message_outbox_send();
  }
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  int bar_width = (s_battery_level * bounds.size.w) / 100;

  // Draw battery bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, bar_width, bounds.size.h), 0, GCornerNone);

  // Color the bar based on charge level
  #ifdef PBL_COLOR
  if (s_battery_level <= 20) {
    graphics_context_set_fill_color(ctx, GColorRed);
  } else if (s_battery_level <= 40) {
    graphics_context_set_fill_color(ctx, GColorYellow);
  } else {
    graphics_context_set_fill_color(ctx, GColorGreen);
  }
  graphics_fill_rect(ctx, GRect(0, 0, bar_width, bounds.size.h), 0, GCornerNone);
  #endif
}

static void bluetooth_callback(bool connected) {
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if (!connected) {
    vibes_double_pulse();
  }
}

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

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Load custom fonts
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_JERSEY_56));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_JERSEY_24));

  // Time layer
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Date layer
  s_date_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(110, 104), bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  // Weather layer
  s_weather_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(140, 134), bounds.size.w, 30));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));

  // Battery layer
  s_battery_layer = layer_create(GRect(0, 0, bounds.size.w, 3));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_layer);

  // Bluetooth icon layer
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
  s_bt_icon_layer = bitmap_layer_create(GRect(bounds.size.w / 2 - 10, 10, 20, 20));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_icon_layer));

  // Show current connection state
  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_layer);
  layer_destroy(s_battery_layer);
  bitmap_layer_destroy(s_bt_icon_layer);
  gbitmap_destroy(s_bt_icon_bitmap);
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  update_time();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
