# Live API documentation

The SDK reference lives at https://developer.repebble.com. Do not rely on
memory for function signatures, enum names, or platform availability.
Fetch the page instead.

Every page on the site has a Markdown twin: append `.md` to the page URL.
C API pages end in `/index.md`.

- Index of every page, with one-line descriptions:
  https://developer.repebble.com/llms.txt
- Example: https://developer.repebble.com/guides/user-interfaces/layers
  becomes https://developer.repebble.com/guides/user-interfaces/layers.md

Last checked against SDK 4.33.1 and pebble tool 5.0.40.

## C SDK

| Topic | URL |
|---|---|
| Window, Window Stack | https://developer.repebble.com/docs/c/User_Interface/Window/index.md |
| Layers (Layer, TextLayer, BitmapLayer, MenuLayer, ...) | https://developer.repebble.com/docs/c/User_Interface/Layers/index.md |
| TextLayer | https://developer.repebble.com/docs/c/User_Interface/Layers/TextLayer/index.md |
| MenuLayer | https://developer.repebble.com/docs/c/User_Interface/Layers/MenuLayer/index.md |
| Clicks (buttons) | https://developer.repebble.com/docs/c/User_Interface/Clicks/index.md |
| UnobstructedArea (Quick View) | https://developer.repebble.com/docs/c/User_Interface/UnobstructedArea/index.md |
| Graphics Context | https://developer.repebble.com/docs/c/Graphics/Graphics_Context/index.md |
| Drawing Primitives | https://developer.repebble.com/docs/c/Graphics/Drawing_Primitives/index.md |
| Drawing Paths (GPath) | https://developer.repebble.com/docs/c/Graphics/Drawing_Paths/index.md |
| Drawing Text | https://developer.repebble.com/docs/c/Graphics/Drawing_Text/index.md |
| Graphics Types (GPoint, GRect, GColor) | https://developer.repebble.com/docs/c/Graphics/Graphics_Types/index.md |
| Color Definitions (all 64 GColor names) | https://developer.repebble.com/docs/c/Graphics/Graphics_Types/Color_Definitions/index.md |
| Fonts | https://developer.repebble.com/docs/c/Graphics/Fonts/index.md |
| TickTimerService | https://developer.repebble.com/docs/c/Foundation/Event_Service/TickTimerService/index.md |
| AccelerometerService (taps) | https://developer.repebble.com/docs/c/Foundation/Event_Service/AccelerometerService/index.md |
| AppMessage | https://developer.repebble.com/docs/c/Foundation/AppMessage/index.md |
| Memory Management | https://developer.repebble.com/docs/c/Foundation/Memory_Management/index.md |

The rest of the C SDK (Timer, Math, Storage, Wakeup, Battery, Connection,
Vibes, Light, App Glance, Health, Worker) is listed under "C SDK" in
`llms.txt`.

## Guides

| Topic | URL |
|---|---|
| Layers | https://developer.repebble.com/guides/user-interfaces/layers.md |
| Unobstructed Area | https://developer.repebble.com/guides/user-interfaces/unobstructed-area.md |
| Round App UI | https://developer.repebble.com/guides/user-interfaces/round-app-ui.md |
| Drawing Primitives, Images and Text | https://developer.repebble.com/guides/graphics-and-animations/drawing-primitives-images-and-text.md |
| Animations | https://developer.repebble.com/guides/graphics-and-animations/animations.md |
| Vector Graphics | https://developer.repebble.com/guides/graphics-and-animations/vector-graphics.md |
| Fonts | https://developer.repebble.com/guides/app-resources/fonts.md |
| System Fonts (names and sizes) | https://developer.repebble.com/guides/app-resources/system-fonts.md |
| Buttons | https://developer.repebble.com/guides/events-and-services/buttons.md |
| Persistent Storage | https://developer.repebble.com/guides/events-and-services/persistent-storage.md |
| PebbleKit JS | https://developer.repebble.com/guides/communication/using-pebblekit-js.md |
| `Pebble` object (PebbleKit JS API) | https://developer.repebble.com/docs/pebblekit-js/Pebble.md |
| The `pebble` tool | https://developer.repebble.com/guides/tools-and-resources/pebble-tool.md |
| Debugging with App Logs | https://developer.repebble.com/guides/debugging/debugging-with-app-logs.md |

## Alloy (JavaScript on the watch)

| Topic | URL |
|---|---|
| Getting Started | https://developer.repebble.com/guides/alloy/getting-started.md |
| Watchfaces | https://developer.repebble.com/guides/alloy/watchfaces.md |
| Poco Graphics | https://developer.repebble.com/guides/alloy/poco-guide.md |
| Piu UI Framework | https://developer.repebble.com/guides/alloy/piu-guide.md |
| Sensors and Input | https://developer.repebble.com/guides/alloy/sensors-and-input.md |
| Networking | https://developer.repebble.com/guides/alloy/networking.md |
| Storage | https://developer.repebble.com/guides/alloy/storage.md |
| App Messages | https://developer.repebble.com/guides/alloy/app-messages.md |
| Animations | https://developer.repebble.com/guides/alloy/animations.md |
| Native Functions (FFI) | https://developer.repebble.com/guides/alloy/ffi.md |

## Tutorials

| Tutorial | URL |
|---|---|
| C watchface, part 1 (time and date) | https://developer.repebble.com/tutorials/watchface-tutorial/part1.md |
| C watchface, part 4 (weather via PebbleKit JS) | https://developer.repebble.com/tutorials/watchface-tutorial/part4.md |
| C watchface, part 6 (Clay settings) | https://developer.repebble.com/tutorials/watchface-tutorial/part6.md |
| Alloy watchface, part 1 | https://developer.repebble.com/tutorials/alloy-watchface-tutorial/part1.md |
| Alloy watchface, part 4 (weather via `fetch()`) | https://developer.repebble.com/tutorials/alloy-watchface-tutorial/part4.md |

The source for the C tutorial parts is in `tutorials/c-watchface-tutorial/`
when this skill is used from a clone of its repository.
