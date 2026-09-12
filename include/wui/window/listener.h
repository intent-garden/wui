//
// Copyright (c) 2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/ud84/wui
//

#pragma once

#include <wui/window/window.hpp>

#include <wui/system/system_context.hpp>
#include <wui/common/error.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>

#ifdef __linux__
#include <xcb/xcb.h>
#endif

namespace wui
{

#ifdef __linux__

struct wnd
{
    std::shared_ptr<window> window_;
    bool created = false;
};

class listener
{
public:
    listener();
    ~listener();

    void add_window(xcb_window_t id, std::shared_ptr<window> window);
    void delete_window(xcb_window_t id);

    bool init();
    
    system_context const &context() const;

    error const &get_error() const;

    /// Opaque and stable across cancellations; 0 == invalid.
    using timer_id = uint64_t;
    static constexpr timer_id kInvalidTimer = 0;

    /// Registers a periodic callback invoked on the window event thread
    /// (listener thread) every `interval`. Several timers can coexist; each
    /// registration returns a cancellation token. Scheduling is thread-safe:
    /// the state is guarded by a mutex and an eventfd wakes the listener so a
    /// short first tick does not wait for the next poll pass.
    timer_id schedule_timer(std::chrono::milliseconds interval,
                            std::function<void()> callback);
    /// Cancels a timer. Returns false when the id is not registered.
    ///
    /// Cancellation is asynchronous with respect to a callback that is already
    /// running: clear_timer() does not wait for it to finish, so objects
    /// captured by a running callback (raw pointers/references) must not be
    /// considered safe to release just because the timer was cancelled. Use
    /// shared ownership or an explicit lifetime token when the callback may
    /// outlive the cancellation point.
    bool clear_timer(timer_id id);
    /// Number of active timers (for tests/introspection).
    size_t timer_count() const;

private:
    std::atomic<bool> started{false};
    std::thread thread;
    system_context context_;

    std::unordered_map<xcb_window_t, wnd> windows;

    error err;

    void start();
    void stop();

    void process_events();

 private:
    struct Timer
    {
        std::chrono::steady_clock::time_point next;
        std::chrono::milliseconds interval;
        std::function<void()> callback;
    };

    bool drain_xcb_events();
    void dispatch_due_timers();
    void wake_timer_thread();

    int wakeup_fd_{-1};
    mutable std::mutex timer_mutex_;
    std::unordered_map<timer_id, Timer> timers_;
    timer_id next_timer_id_{kInvalidTimer + 1};
};

listener& get_listener(); /// Singleton

#else // Windows - stub implementation

class listener
{
public:
    using timer_id = uint64_t;
    static constexpr timer_id kInvalidTimer = 0;

    listener() {}
    ~listener() {}
    void add_window(void*, std::shared_ptr<window>) {}
    void delete_window(void*) {}
    bool init() { return true; }
    system_context const &context() const { static system_context ctx{}; return ctx; }
    error const &get_error() const { static error e{}; return e; }
    /// Listener timers are not supported on this platform: registration
    /// explicitly returns an invalid token (no silent stubs) so callers can
    /// degrade knowingly.
    timer_id schedule_timer(std::chrono::milliseconds, std::function<void()>) {
        return kInvalidTimer;
    }
    bool clear_timer(timer_id) { return false; }
    size_t timer_count() const { return 0; }
};

inline listener& get_listener() { static listener instance; return instance; }

#endif

}
