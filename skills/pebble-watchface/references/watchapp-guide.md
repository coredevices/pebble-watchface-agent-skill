# Watchapp Guide (C)

Watchapps differ from watchfaces in one config flag plus interaction model. Build, wscript, emulator flow all identical.

## Config Difference

package.json: `"watchapp": { "watchface": false }`. That's it. Same `main()`/`init()`/`deinit()`/`app_event_loop()` skeleton.

| | Watchface | Watchapp |
|---|---|---|
| Launch | System (default face) | Launcher, Quick Launch, phone, wakeup, timeline action, worker |
| Exit | System (user presses button to leave) | BACK pops windows; app exits when stack empties; long-press BACK force-quits (not overridable) |
| Buttons | **None** — use accel taps | Full ClickHandler API |
| Status bar | Never | Optional StatusBarLayer |

`launch_reason()` → `APP_LAUNCH_USER/PHONE/WAKEUP/WORKER/QUICK_LAUNCH/TIMELINE_ACTION/...`
`exit_reason_set(APP_EXIT_ACTION_PERFORMED_SUCCESSFULLY)` before exit → returns user to watchface instead of launcher (one-shot action apps).

## Button Input

```c
static void select_click_handler(ClickRecognizerRef recognizer, void *context) { }

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_repeat_handler);  // fires while held
  window_long_click_subscribe(BUTTON_ID_DOWN, 500, long_down_handler, NULL);
  window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 2, 300, true, double_click_handler);
  window_raw_click_subscribe(BUTTON_ID_UP, down_handler, up_handler, NULL);       // raw down/up for games
}

// in window setup:
window_set_click_config_provider(window, click_config_provider);
```

- Buttons: `BUTTON_ID_BACK/UP/SELECT/DOWN`.
- BACK: single-click overridable; no repeating/long handlers; long-press always terminates.
- `click_recognizer_get_button_id(recognizer)`, `click_number_of_clicks_counted(recognizer)` inside handlers.

## Window Stack (multi-screen apps)

```c
window_stack_push(window, true);     // animated slide
window_stack_pop(true);
window_stack_pop_all(true);
window_stack_get_top_window();
```

Each screen = one Window with own load/unload + click config provider. Push detail window on select; BACK pops automatically; app exits when stack empties.

## UI Components

### MenuLayer (scrolling list)

```c
static uint16_t get_num_rows(MenuLayer *ml, uint16_t section, void *ctx) { return 5; }
static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *idx, void *data) {
  menu_cell_basic_draw(ctx, cell, "Title", "Subtitle", NULL);  // NULL = no icon
}
static void select_cb(MenuLayer *ml, MenuIndex *idx, void *ctx) { /* push detail window */ }

// window load:
s_menu = menu_layer_create(layer_get_bounds(window_get_root_layer(window)));
menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks) {
  .get_num_rows = get_num_rows, .draw_row = draw_row, .select_click = select_cb,
});
menu_layer_set_click_config_onto_window(s_menu, window);   // wires UP/DOWN/SELECT
layer_add_child(window_get_root_layer(window), menu_layer_get_layer(s_menu));
// unload: menu_layer_destroy(s_menu);
```

### Menu → Detail data passing (standard idiom)

One reusable detail Window + a static selected-index; select stores the index and pushes; detail's load/update reads it:

```c
static int s_selected;
static Window *s_detail_window;

static void select_cb(MenuLayer *ml, MenuIndex *idx, void *ctx) {
  s_selected = idx->row;
  window_stack_push(s_detail_window, true);        // create once in init, reuse
}
// detail window .appear handler (fires every push):
static void detail_appear(Window *w) {
  text_layer_set_text(s_name_layer, ITEMS[s_selected].name);
  layer_mark_dirty(s_drawing_layer);               // update proc reads s_selected
}
```

Use `.appear` (not `.load`) for per-selection refresh — `.load` only fires on first push of a reused window.

### SimpleMenuLayer (static lists — less boilerplate)

`simple_menu_layer_create(frame, window, sections, num_sections, ctx)` with static (long-lived) `SimpleMenuSection{title, items, num_items}` / `SimpleMenuItem{title, subtitle, icon, callback}` arrays.

### ActionBarLayer (right-edge icon bar)

```c
s_action_bar = action_bar_layer_create();
action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_up);
action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);
action_bar_layer_add_to_window(s_action_bar, window);
// Content layers: width = bounds.size.w - ACTION_BAR_WIDTH (30 std, 34 emery/gabbro, 40 chalk)
```

### StatusBarLayer

```c
s_status_bar = status_bar_layer_create();   // STATUS_BAR_LAYER_HEIGHT (16 on rect)
status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);
layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
// Content below: GRect(0, STATUS_BAR_LAYER_HEIGHT, w, h - STATUS_BAR_LAYER_HEIGHT)
```

### ScrollLayer

`scroll_layer_create(frame)` + `scroll_layer_set_content_size(sl, GSize(w, content_h))` (manual!) + `scroll_layer_add_child` + `scroll_layer_set_click_config_onto_window` (UP/DOWN auto-scroll).

### ActionMenu (hierarchical action picker)

`action_menu_level_create(n)` → `action_menu_level_add_action(level, "Label", cb, data)` → `action_menu_open(&(ActionMenuConfig){.root_level=..., .colors={...}})`. Destroy hierarchy with `action_menu_hierarchy_destroy`.

### Dialogs

No Dialog API. Pattern: Window + TextLayer (+ BitmapLayer icon), dismissed via BACK or AppTimer. `NumberWindow` is the only prebuilt input window.

## Game Loop Pattern

```c
#define FRAME_MS 33   // ~30fps

static void game_tick(void *data) {
  update_game_state();               // physics, read held-button flags
  layer_mark_dirty(s_game_layer);    // schedules redraw of LayerUpdateProc
  s_timer = app_timer_register(FRAME_MS, game_tick, NULL);
}

// drawing:
static void game_draw(Layer *layer, GContext *ctx) { /* graphics_* calls */ }
layer_set_update_proc(s_game_layer, game_draw);

// input: raw clicks set/clear flags read by game_tick
static void up_down(ClickRecognizerRef r, void *ctx) { s_up_held = true; }
static void up_up(ClickRecognizerRef r, void *ctx) { s_up_held = false; }
window_raw_click_subscribe(BUTTON_ID_UP, up_down, up_up, NULL);

// start in window appear; STOP in window disappear/unload:
app_timer_cancel(s_timer);
```

Tweens (non-game): `PropertyAnimation` via `property_animation_create_layer_frame(layer, &from, &to)` + `animation_schedule()`; custom via `AnimationImplementation.update` receiving 0..65535 progress.

Drawing note: the SDK has **no ellipse primitive**. Rings: `graphics_fill_circle` then punch out with a background-color fill_circle (or `graphics_draw_circle` for 1px outline). Tilted/elliptical shapes (e.g. Saturn's ring): GPath polygon, or stacked 1px horizontal lines with per-row width.

## App-Only Capabilities

### Persistent storage

```c
persist_write_int(KEY_SCORE, score);
int score = persist_read_int(KEY_SCORE);         // 0 if unset
persist_exists(KEY); persist_write_string/data/bool(...); persist_delete(KEY);
```

256 B max per value (`PERSIST_DATA_MAX_LENGTH`), ~4 KB total per app. Save game state in window disappear. (Also works in watchfaces.)

### Wakeup (scheduled app launches)

```c
WakeupId id = wakeup_schedule(timestamp, cookie, true);
wakeup_service_subscribe(wakeup_handler);        // if app running when it fires
// at launch: if (launch_reason() == APP_LAUNCH_WAKEUP) wakeup_get_launch_event(&id, &cookie);
```

Max 8 scheduled; none within 1 min of another; store WakeupIds in persist.

### App Glances (launcher subtitle)

```c
app_glance_reload(glance_reload_cb, NULL);   // in window unload / before exit
// in cb: app_glance_add_slice(session, (AppGlanceSlice){
//   .layout = {.icon = ..., .subtitle_template_string = "..."},
//   .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION});
```

Not on aplite.

### Background workers

`worker_src/c/worker.c` with `#include <pebble_worker.h>`, own `main()` + `worker_event_loop()`. **10.5 kB limit**, no UI/AppMessage/resources. `app_worker_launch()/kill()`, AppWorkerMessage for live comms, shared persist storage.

### Web APIs (AppMessage + pkjs)

Same as watchfaces — see SKILL.md weather section. Apps commonly use bigger buffers: `app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum())` (minimums: inbox 124 B, outbox 636 B). Register callbacks before open.

## Memory Limits (code + heap)

| Platform | Limit |
|---|---|
| aplite | 24 KB |
| basalt / chalk / diorite / flint | 64 KB |
| emery / gabbro | 128 KB |
| background worker | 10.5 KB |

Use `heap_bytes_free()` when debugging. Per-platform code: `PBL_IF_ROUND_ELSE()`, `PBL_IF_COLOR_ELSE()`, `#ifdef PBL_PLATFORM_EMERY`. Always use `ACTION_BAR_WIDTH`/`STATUS_BAR_LAYER_HEIGHT` macros, never hardcode.
