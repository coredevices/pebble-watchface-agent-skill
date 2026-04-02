#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
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
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  int bar_width = (s_battery_level * bounds.size.w) / 100;

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, bar_width, bounds.size.h), 0, GCornerNone);

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

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_JERSEY_56));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_JERSEY_24));

  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_date_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(110, 104), bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

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

  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
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
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
