# macOS

The macOS backend uses AppKit, Core Graphics and ImageIO. It does not require
X11 or Cairo. Existing C++ controls and the by-value `rect` API are preserved.

## Build

Install Xcode Command Line Tools and CMake 3.16 or later. The default macOS
deployment target is 10.15. Runtime validation is performed on Apple Silicon;
Intel and universal builds can be produced with `CMAKE_OSX_ARCHITECTURES`.

```sh
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release
cmake --build build-macos --parallel
open build-macos/examples/demo/demo.app
```

The `hello_world` and `simple` examples also build as application bundles.
Resources are copied into `Contents/Resources/res`. Relative resource paths are
resolved against the working directory first, then the application bundle.
The library does not change the working directory. Use an explicit writable path
for INI configuration when launching from Finder, for example in Application Support.

```sh
cmake -S . -B build-macos-universal -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DWUI_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-macos-universal --parallel
```

Consumers can use `add_subdirectory(path/to/wui)` and
`target_link_libraries(my_app PRIVATE wui)`. Public headers remain plain C++;
Objective-C++ and Apple frameworks are handled by the `wui` target.

## Behavior

Run initialization, the event loop, and UI operations on the main thread.
Control and timer callbacks run on that thread. `window::emit_event()` can be
called from a worker thread and delivers asynchronously on the main thread.

Coordinates are logical points; bitmap buffers follow the display's Retina
scale. Font size follows WUI's line-height convention. Text measurement does not
require an open window. The backend supports UTF-8 and IME composition through
`NSTextInputClient`, Cmd+A/C/X/V, Tab/Shift+Tab, mouse input, vertical scrolling,
embedded and physical modal windows, moving, resizing and minimizing.
Window decorations use the existing WUI controls.

Colors retain WUI's POSIX transparency convention: the fourth `make_color`
argument is `0` for opaque and `255` for transparent. `tray_icon` uses an
NSStatusItem; `show_message()` displays a popover, not a Notification Center alert.

## Tests and limitations

```sh
cmake -S . -B build-macos -DWUI_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-macos --parallel
ctest --test-dir build-macos --output-on-failure
```

Tests require an active macOS graphical session. They open temporary windows,
exercise events, UTF-8, clipboard, dialogs, timers and bitmap pixels, and launch
all three examples. Screenshots are saved as `/tmp/wui-macos-*.png`.

Device hotplug notifications are not implemented;
`enable_device_change_handling(true)` reports this through `window::get_error()`.
Per-window taskbar visibility has no equivalent in the application-wide Dock.
Full control accessibility and horizontal trackpad scrolling are not yet implemented.

The optional [native media devices API](media.md) adds microphone, audio output, camera and screen capture for applications that need them.
