//
// Copyright (c) 2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/ud84/wui
//

#include <wui/window/listener.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <vector>


namespace wui
{

namespace
{
// Events processed per pass: bounds timer latency under a continuous stream
// of X events (the loop returns and due timers are dispatched between
// batches).
constexpr int kMaxEventsPerBatch = 64;
}

listener::listener()
    : context_{},
    windows{},
    err{}
{
}

listener::~listener()
{
    stop();
}

void listener::add_window(xcb_window_t id, std::shared_ptr<window> window)
{
    {
        std::lock_guard<std::mutex> lock(windows_mutex_);
        windows.emplace(id, std::move(window));
    }
    wake_timer_thread();
}

void listener::delete_window(xcb_window_t id)
{
    // Release the last shared_ptr outside the registry lock: destruction may
    // unsubscribe, cancel timers, or remove another window.
    std::shared_ptr<window> removed;
    {
        std::lock_guard<std::mutex> lock(windows_mutex_);
        auto it = windows.find(id);
        if (it != windows.end())
        {
            removed = std::move(it->second);
            windows.erase(it);
        }
    }
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        damage_.erase(id);
    }
    // Keep the event thread alive until listener shutdown. Closing the last
    // window must not strand timers or make a subsequent open join itself.
}

std::shared_ptr<window> listener::find_window(xcb_window_t id)
{
    std::lock_guard<std::mutex> lock(windows_mutex_);
    auto it = windows.find(id);
    return it == windows.end() ? nullptr : it->second;
}

bool listener::init()
{
    std::lock_guard<std::mutex> lock(init_mutex_);
    if (context_.display) return started.load();
    // Xlib must know it is going to be used from more than one thread before any
    // other Xlib call. This display is polled by the listener thread while the
    // application creates and destroys windows from its own thread on the same
    // connection; without this the shared Xlib/XCB state is not protected and
    // the process dies inside xcb_send_request.
    XInitThreads();

    context_.display = XOpenDisplay(NULL);
    if (!context_.display)
    {
        err.type = error_type::system_error;
        err.component = "listener::start()";
        err.message = "Can't make the connection to X server";
        
        return false;
    }

    XSetEventQueueOwner(context_.display, XCBOwnsEventQueue);
    context_.connection = XGetXCBConnection(context_.display);

    context_.screen = xcb_setup_roots_iterator(xcb_get_setup(context_.connection)).data;

    // Wake-up channel for the listener thread: scheduling/cancelling timers
    // or stopping writes an event so poll returns immediately instead of
    // sleeping until its timeout (short ticks are not delayed).
    wakeup_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

    if (wakeup_fd_ < 0)
    {
        err = {error_type::system_error, "listener::init()", "Cannot create eventfd"};
        XCloseDisplay(context_.display);
        context_ = {};
        return false;
    }
    start();

    return true;
}

void listener::start()
{
    if (started.exchange(true))
    {
        return;
    }

    thread = std::thread(std::bind(&listener::process_events, this));
}

void listener::stop()
{
    started = false;
    wake_timer_thread();
    if (thread.joinable()) thread.join();

    const int wakeup = wakeup_fd_.exchange(-1);
    if (wakeup >= 0) close(wakeup);
    if (context_.display)
    {
        XCloseDisplay(context_.display);
        context_.display = nullptr;
        context_.connection = nullptr;
    }
}

system_context const &listener::context() const
{
    return context_;
}

void listener::wake_timer_thread()
{
    const int wakeup = wakeup_fd_.load();
    if (wakeup < 0) return;
    const uint64_t one = 1;
    ssize_t result;
    do { result = write(wakeup, &one, sizeof(one)); } while (result < 0 && errno == EINTR);  // EAGAIN is fine: the counter is already set
}

void listener::process_events()
{
    const int fd = xcb_get_file_descriptor(context_.connection);
    xcb_flush(context_.connection);

    while (started)
    {
        // 1) Due timers (independent; rescheduling from inside the callback
        //    included - see dispatch_due_timers).
        const bool more_tasks = dispatch_tasks();
        dispatch_due_timers();

        // 2) The XCB queue is ALWAYS drained before waiting: events can
        //    already sit in XCB's internal queue while the socket is empty
        //    (e.g. after a synchronous wait), so this does not depend on
        //    POLLIN.
        const bool events = drain_xcb_events();
        paint_pending();
        if (!started) break;
        if (events || more_tasks) continue;

        // 3) Wait with a timeout until the next timer (or 100 ms idle).
        int timeout_ms = 100;
        {
            std::lock_guard<std::mutex> lock(timer_mutex_);
            if (!timers_.empty())
            {
                auto next = std::chrono::steady_clock::time_point::max();
                for (const auto& [id, timer] : timers_)
                    next = std::min(next, timer.next);
                const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                    next - std::chrono::steady_clock::now()).count();
                timeout_ms = static_cast<int>(std::clamp<long long>(remaining, 1, 1000));
            }
        }

        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            if (!tasks_.empty()) timeout_ms = 0;
            else if (!damage_.empty())
            {
                const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                    next_paint_ - std::chrono::steady_clock::now()).count();
                timeout_ms = std::min(timeout_ms, static_cast<int>(std::max<long long>(0, remaining)));
            }
        }

        pollfd pfd[2]{};
        pfd[0].fd = fd;
        pfd[0].events = POLLIN;
        nfds_t count = 1;
        if (wakeup_fd_ >= 0)
        {
            pfd[1].fd = wakeup_fd_;
            pfd[1].events = POLLIN;
            count = 2;
        }

        const int rc = poll(pfd, count, timeout_ms);
        if (rc < 0)
        {
            if (errno == EINTR) continue;
            started = false;
            break;
        }

        if (rc > 0)
        {
            // X connection error/close: do not spin in a hot loop; end the
            // loop just like xcb_wait_for_event returning nullptr did.
            if (pfd[0].revents & (POLLHUP | POLLERR | POLLNVAL))
            {
                started = false;
                break;
            }

            // Wake-up consumed (timers rescheduled or stop()).
            if (count == 2 && (pfd[1].revents & (POLLIN | POLLHUP | POLLERR)))
            {
                uint64_t value = 0;
                while (wakeup_fd_ >= 0 && read(wakeup_fd_, &value, sizeof(value)) > 0)
                {
                }
            }

        }

        if (xcb_connection_has_error(context_.connection))
        {
            started = false;
            break;
        }
    }
}

void listener::post(std::function<void()> callback)
{
    if (!callback) return;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        tasks_.push_back(std::move(callback));
    }
    wake_timer_thread();
}

bool listener::dispatch_tasks()
{
    std::deque<std::function<void()>> ready;
    bool more;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        for (int i = 0; i < kMaxEventsPerBatch && !tasks_.empty(); ++i)
        {
            ready.push_back(std::move(tasks_.front()));
            tasks_.pop_front();
        }
        more = !tasks_.empty();
    }
    for (auto& callback : ready) callback();
    return more;
}

void listener::request_redraw(xcb_window_t id, rect area, bool clear)
{
    if (!id || area.width() <= 0 || area.height() <= 0) return;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        auto [it, inserted] = damage_.emplace(id, Damage{area, clear});
        if (!inserted)
        {
            auto& pending = it->second;
            pending.area = {std::min(pending.area.left, area.left),
                std::min(pending.area.top, area.top), std::max(pending.area.right, area.right),
                std::max(pending.area.bottom, area.bottom)};
            pending.clear = pending.clear || clear;
        }
    }
    wake_timer_thread();
}

void listener::paint_pending()
{
    auto now = std::chrono::steady_clock::now();
    if (now < next_paint_) return;
    std::unordered_map<xcb_window_t, Damage> ready;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        ready.swap(damage_);
    }
    if (ready.empty()) return;
    next_paint_ = now + std::chrono::milliseconds(16);
    for (const auto& [id, damage] : ready)
    {
        // Keep the target alive throughout the callback, even when it removes
        // itself (or another window) from the registry.
        auto target = find_window(id);
        if (!target) continue;
        const auto size = target->position();
        rect area{std::max(0, damage.area.left), std::max(0, damage.area.top),
            std::min(size.width(), damage.area.right), std::min(size.height(), damage.area.bottom)};
        if (area.width() <= 0 || area.height() <= 0) continue;
        xcb_expose_event_t event{};
        event.response_type = XCB_EXPOSE;
        event.window = id;
        event.x = static_cast<uint16_t>(area.left);
        event.y = static_cast<uint16_t>(area.top);
        event.width = static_cast<uint16_t>(area.width());
        event.height = static_cast<uint16_t>(area.height());
        event.pad0 = damage.clear ? 1 : 0;
        target->process_events(reinterpret_cast<xcb_generic_event_t&>(event));
    }
}

bool listener::drain_xcb_events()
{
    // Collapse resize + expose bursts, but flush pending geometry BEFORE any
    // input/client event so clicks are interpreted against the correct layout.
    std::unordered_map<xcb_window_t, xcb_configure_notify_event_t> geometry;
    auto configure = [&] {
        auto ready = std::move(geometry);
        geometry.clear();
        for (auto& [id, event] : ready)
            if (auto target = find_window(id))
                target->process_events(reinterpret_cast<xcb_generic_event_t&>(event));
    };
    int handled = 0;
    while (started && handled < kMaxEventsPerBatch)
    {
        auto* e = xcb_poll_for_event(context_.connection);
        if (!e) break;
        ++handled;
        const auto type = e->response_type & ~0x80;
        if (type == XCB_EXPOSE)
        {
            const auto& ev = *reinterpret_cast<xcb_expose_event_t*>(e);
            request_redraw(ev.window, {ev.x, ev.y, ev.x + ev.width, ev.y + ev.height}, ev.pad0 != 0);
        }
        else if (type == XCB_CONFIGURE_NOTIFY)
        {
            const auto& ev = *reinterpret_cast<xcb_configure_notify_event_t*>(e);
            geometry[ev.window] = ev;
        }
        else
        {
            configure();
            xcb_window_t id = e->pad[2];
            if (type == XCB_PROPERTY_NOTIFY) id = reinterpret_cast<xcb_property_notify_event_t*>(e)->window;
            else if (type == XCB_CLIENT_MESSAGE) id = reinterpret_cast<xcb_client_message_event_t*>(e)->window;
            if (auto target = find_window(id)) target->process_events(*e);
        }
        free(e);
    }
    configure();
    return handled != 0;
}

listener::timer_id listener::schedule_timer(std::chrono::milliseconds interval,
                                            std::function<void()> callback)
{
    if (!callback) return kInvalidTimer;

    const std::chrono::milliseconds safe_interval =
        interval > std::chrono::milliseconds(0) ? interval : std::chrono::milliseconds(16);

    std::lock_guard<std::mutex> lock(timer_mutex_);
    timer_id id = next_timer_id_++;
    if (id == kInvalidTimer) id = next_timer_id_++;  // never return 0
    timers_[id] = { std::chrono::steady_clock::now() + safe_interval,
                    safe_interval,
                    std::move(callback) };
    wake_timer_thread();
    return id;
}

bool listener::clear_timer(timer_id id)
{
    // The callback is extracted under the lock and destroyed AFTER releasing
    // it: its destructor may re-enter clear_timer/schedule_timer (e.g. through
    // a shared_ptr that cancels another timer) and re-acquire the mutex.
    std::optional<Timer> removed;
    {
        std::lock_guard<std::mutex> lock(timer_mutex_);
        auto it = timers_.find(id);
        if (it == timers_.end()) return false;
        removed.emplace(std::move(it->second));
        timers_.erase(it);  // only the already-moved entry is destroyed here
    }
    removed.reset();        // callback destructor runs without the lock
    wake_timer_thread();
    return true;
}

size_t listener::timer_count() const
{
    std::lock_guard<std::mutex> lock(timer_mutex_);
    return timers_.size();
}

void listener::dispatch_due_timers()
{
    const auto now = std::chrono::steady_clock::now();

    // Snapshot of due ids under the lock; callbacks run outside the mutex so
    // schedule_timer/clear_timer may be called from inside a callback without
    // deadlock (and without racing other threads).
    std::vector<timer_id> due;
    {
        std::lock_guard<std::mutex> lock(timer_mutex_);
        due.reserve(timers_.size());
        for (const auto& [id, timer] : timers_)
            if (timer.next <= now) due.push_back(id);
    }

    for (timer_id id : due)
    {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(timer_mutex_);
            auto it = timers_.find(id);
            if (it == timers_.end() || it->second.next > now) continue;
            callback = it->second.callback;
        }

        if (callback) callback();

        // Preserve the periodic deadline; skip missed ticks rather than
        // accumulating callback duration as drift or running a catch-up storm.
        // Self-cancellation/restart creates another id and is left untouched.
        std::lock_guard<std::mutex> lock(timer_mutex_);
        auto it = timers_.find(id);
        if (it != timers_.end())
        {
            const auto finished = std::chrono::steady_clock::now();
            auto& timer = it->second;
            timer.next += timer.interval;
            if (timer.next <= finished)
                timer.next += timer.interval * ((finished - timer.next) / timer.interval + 1);
        }
    }
}

error const &listener::get_error() const
{
    return err;
}

listener& get_listener()
{
    static listener instance;
    return instance;
}

}
