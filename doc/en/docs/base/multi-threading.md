# Threads and timers

WUI does not promise arbitrary concurrent access to controls. Update an interface
from its owning event thread. Internal locks around subscriptions do not make
`add_control()`, drawing or control state generally thread-safe.

| Platform | `timer` callback |
| --- | --- |
| Windows | Owning UI/message-loop thread |
| Linux | Shared X11 event thread |
| macOS | Main UI thread |
| WASM | Browser main thread; background tabs can throttle it |

The actual timer API is `timer(callback)`, `start(interval_ms)` and `stop()`.
Constructing a timer does not start it. Keep it alive while needed. For portable
UI code, send a small notification to the window and update controls in its event
handler, rather than modifying controls directly in a timer/worker callback.

```cpp
#include <wui/system/timer.hpp>

wui::timer timer([weak = std::weak_ptr<wui::window>(window)] {
    if (auto target = weak.lock()) target->emit_event(100, 0);
});
// Handle internal_event_type::user_emitted in the window subscription.
timer.start(100); // milliseconds
// Before captured state or the receiving window is destroyed:
timer.stop();
```

`emit_event(x,y)` delivers an `internal_event_type::user_emitted` event. Transfer
larger data using application-owned storage with appropriate synchronization;
the two integer arguments are not object ownership. Join workers and stop timers
before destroying the data they capture.

On Linux, `timer` callbacks, X11 events, device-change notifications and painting
share the listener thread. Timer intervals use a steady clock; missed ticks are
skipped rather than delivered in a catch-up burst. Callbacks must remain short:
move decoding, I/O and other expensive work to a worker.

Linux `timer::stop()` prevents further invocations and waits for an in-flight
callback when called from another thread. A callback can stop, restart or destroy
its own timer. Stop timers before destroying objects they capture by raw pointer;
the timer cannot extend those objects' lifetime automatically. Do not hold a
lock needed by the callback while waiting in `stop()`.

Linux worker code can use `get_listener().post(callback)` (include
`<wui/window/listener.h>`) to schedule UI work. Capture the receiver through a
`weak_ptr`, and lock it inside the queued callback. `post()` is asynchronous and
does not make other direct access to control state safe. The lower-level
`listener::clear_timer()` is also asynchronous; use `wui::timer` when synchronized
cancellation is needed.

The WASM backend currently has no WUI pthread/Worker integration. On macOS all UI
initialization, event processing and drawing must run on the main thread.

On Windows, construct `timer` on the thread that owns the UI message loop and
keep that loop running while callbacks are needed. `start()` and `stop()` can
be called from workers; native timer changes are queued back to the owner.
`stop()` invalidates pending ticks immediately and waits for an in-flight
callback from another thread. Self-stop, restart and self-destruction are
supported. Do not hold a lock needed by the callback while waiting in `stop()`.
A timer callback must remain short; perform blocking work on a separate worker.

The Windows backend uses message-only windows and `WM_TIMER`, including its
normal scheduling granularity (typically at least 10 ms). Cancellation filters
stale messages even after a restart. Scrollbar animation also runs in short UI
ticks, without joining an animation worker from mouse-event handlers. Rebuild
the library and applications together after updating the timer implementation.
