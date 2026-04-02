#include <pebble.h>

// Settings storage key
#define SETTINGS_KEY 1

// Settings struct
typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor TextColor;
  bool TemperatureUnit;  // false = Celsius, true = Fahrenheit
  bool ShowDate;
} ClaySettings;

static ClaySettings settings;

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static Layer *s_battery_layer;
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;
static GFont s_time_font;
static GFont s_date_font;
static Layer *s_window_layer;

static int s_battery_level;

static void prv_default_settings() {
  settings.BackgroundColor = GColorBlack;
  settings.TextColor = GColorWhite;
  settings.TemperatureUnit = false;
  settings.ShowDate = true;
}

static void prv_load_settings() {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_update_display() {
  window_set_background_color(s_main_window, settings.BackgroundColor);
  text_layer_set_text_color(s_time_layer, settings.TextColor);
  text_layer_set_text_color(s_date_layer, settings.TextColor);
  text_layer_set_text_color(s_weather_layer, settings.TextColor);
  layer_set_hidden(text_layer_get_layer(s_date_layer), !settings.ShowDate);
  layer_mark_dirty(s_battery_layer);
}

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

  #ifdef PBL_COLOR
  if (s_battery_level <= 20) {
    graphics_context_set_fill_color(ctx, GColorRed);
  } else if (s_battery_level <= 40) {
    graphics_context_set_fill_color(ctx, GColorYellow);
  } else {
    graphics_context_set_fill_color(ctx, GColorGreen);
  }
  #else
  graphics_context_set_fill_color(ctx, settings.TextColor);
  #endif
  graphics_fill_rect(ctx, GRect(0, 0, bar_width, bounds.size.h), 0, GCornerNone);

  // Border
  graphics_context_set_stroke_color(ctx, settings.TextColor);
  graphics_draw_rect(ctx, GRect(0, 0, bounds.size.w, bounds.size.h));
}

static void bluetooth_callback(bool connected) {
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
  if (!connected) {
    vibes_double_pulse();
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Check for Clay settings
  Tuple *bg_color_t = dict_find(iterator, MESSAGE_KEY_BackgroundColor);
  if (bg_color_t) {
    settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);
  }
  Tuple *text_color_t = dict_find(iterator, MESSAGE_KEY_TextColor);
  if (text_color_t) {
    settings.TextColor = GColorFromHEX(text_color_t->value->int32);
  }
  Tuple *temp_unit_t = dict_find(iterator, MESSAGE_KEY_TemperatureUnit);
  if (temp_unit_t) {
    settings.TemperatureUnit = temp_unit_t->value->int32 == 1;
  }
  Tuple *show_date_t = dict_find(iterator, MESSAGE_KEY_ShowDate);
  if (show_date_t) {
    settings.ShowDate = show_date_t->value->int32 == 1;
  }

  // Check for weather data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

  if (temp_tuple && conditions_tuple) {
    static char temperature_buffer[8];
    static char conditions_buffer[32];
    static char weather_layer_buffer[42];

    int temp_val = (int)temp_tuple->value->int32;
    if (settings.TemperatureUnit) {
      temp_val = (temp_val * 9 / 5) + 32;
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d\u00b0F", temp_val);
    } else {
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d\u00b0C", temp_val);
    }
    snprintf(conditions_buffer, sizeof(conditions_buffer),
             "%s", conditions_tuple->value->cstring);
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer),
             "%s %s", temperature_buffer, conditions_buffer);

    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }

  prv_save_settings();
  prv_update_display();
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

static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area, void *context) {
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), true);
}

static void prv_unobstructed_change(AnimationProgress progress, void *context) {
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);

  int time_y = PBL_IF_ROUND_ELSE(
    bounds.size.h / 2 - 32,
    bounds.size.h / 2 - 36);
  layer_set_frame(text_layer_get_layer(s_time_layer),
    GRect(0, time_y, bounds.size.w, 60));

  int date_y = time_y + 52;
  layer_set_frame(text_layer_get_layer(s_date_layer),
    GRect(0, date_y, bounds.size.w, 30));

  int weather_y = date_y + 28;
  layer_set_frame(text_layer_get_layer(s_weather_layer),
    GRect(0, weather_y, bounds.size.w, 30));
}

static void prv_unobstructed_did_change(void *context) {
  GRect full_bounds = layer_get_bounds(s_window_layer);
  GRect unob_bounds = layer_get_unobstructed_bounds(s_window_layer);
  bool is_obstructed = !grect_equal(&full_bounds, &unob_bounds);

  if (!is_obstructed) {
    bluetooth_callback(connection_service_peek_pebble_app_connection());
  }
}

static void main_window_load(Window *window) {
  s_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(s_window_layer);

  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_JERSEY_56));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_JERSEY_24));

  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, settings.TextColor);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(s_window_layer, text_layer_get_layer(s_time_layer));

  s_date_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(110, 104), bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, settings.TextColor);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));

  s_weather_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(140, 134), bounds.size.w, 30));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, settings.TextColor);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");
  layer_add_child(s_window_layer, text_layer_get_layer(s_weather_layer));

  s_battery_layer = layer_create(GRect(0, 0, bounds.size.w, 3));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(s_window_layer, s_battery_layer);

  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
  s_bt_icon_layer = bitmap_layer_create(GRect(bounds.size.w / 2 - 10, 10, 20, 20));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpSet);
  layer_add_child(s_window_layer, bitmap_layer_get_layer(s_bt_icon_layer));

  bluetooth_callback(connection_service_peek_pebble_app_connection());

  UnobstructedAreaHandlers handlers = {
    .will_change = prv_unobstructed_will_change,
    .change = prv_unobstructed_change,
    .did_change = prv_unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);

  prv_update_display();
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
  prv_load_settings();

  s_main_window = window_create();
  window_set_background_color(s_main_window, settings.BackgroundColor);
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

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  app_message_open(256, 256);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
