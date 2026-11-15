// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#ifdef __linux__
#include <wui/system/timer.hpp>
#include <wui/window/listener.h>
#include <mutex>
#include <utility>

namespace wui
{
struct timer::state
{
    // Recursive so callbacks can stop/restart or destroy their own timer.
    std::recursive_mutex mutex;
    std::function<void()> callback;
    listener::timer_id id = listener::kInvalidTimer;
    uint64_t generation = 0;
    bool active = false;
};

timer::timer(std::function<void()> callback) : state_(std::make_shared<state>())
{
    state_->callback = std::move(callback);
    // Ensure the listener is constructed before this timer, so static timers
    // are destroyed before the event loop singleton.
    (void)get_listener();
}

timer::~timer() { stop(); }

void timer::start(uint32_t interval)
{
    const auto shared = state_;
    std::lock_guard<std::recursive_mutex> lock(shared->mutex);
    if (shared->active) return;
    shared->active = true;
    const auto generation = ++shared->generation;
    shared->id = get_listener().schedule_timer(std::chrono::milliseconds(interval), [shared, generation] {
        std::lock_guard<std::recursive_mutex> lock(shared->mutex);
        if (!shared->active || shared->generation != generation) return;
        // Keep the callable alive if it destroys this timer from inside itself.
        auto callback = shared->callback;
        if (callback) callback();
    });
}

void timer::stop()
{
    const auto shared = state_;
    std::lock_guard<std::recursive_mutex> lock(shared->mutex);
    shared->active = false;
    ++shared->generation; // Also invalidate a callback copied before cancellation.
    const auto id = shared->id;
    shared->id = listener::kInvalidTimer;
    if (id != listener::kInvalidTimer) get_listener().clear_timer(id);
}
}
#endif
