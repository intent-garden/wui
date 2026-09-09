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
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <vector>


namespace wui
{

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
    auto w = windows.find(id);
    if (w == windows.end())
    {
        windows[id] = { std::move(window), false };
    }
}

void listener::delete_window(xcb_window_t id)
{
    auto w = windows.find(id);
    if (w != windows.end())
    {
        windows.erase(w);
    }

    if (windows.empty())
    {
        started = false;
        wake_timer_thread();
    }
}

bool listener::init()
{
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

    if (wakeup_fd_ >= 0)
    {
        close(wakeup_fd_);
        wakeup_fd_ = -1;
    }
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
    if (wakeup_fd_ < 0) return;
    const uint64_t one = 1;
    (void)write(wakeup_fd_, &one, sizeof(one));  // EAGAIN is fine: the counter is already set
}

void listener::process_events()
{
    const int fd = xcb_get_file_descriptor(context_.connection);
    xcb_flush(context_.connection);

    while (started)
    {
        // 1) Due timers (independent; rescheduling from inside the callback
        //    included - see dispatch_due_timers).
        dispatch_due_timers();

        // 2) The XCB queue is ALWAYS drained before waiting: events can
        //    already sit in XCB's internal queue while the socket is empty
        //    (e.g. after a synchronous wait), so this does not depend on
        //    POLLIN.
        if (drain_xcb_events())
        {
            continue;  // activity: loop again without blocking
        }

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
            break;  // error del sistema: salimos
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

            if (pfd[0].revents & POLLIN)
            {
                drain_xcb_events();
            }
        }

        if (xcb_connection_has_error(context_.connection))
        {
            started = false;
            break;
        }
    }
}

bool listener::drain_xcb_events()
{
    bool processed = false;
    xcb_generic_event_t *e = nullptr;
    while ((e = xcb_poll_for_event(context_.connection)))
    {
        processed = true;
        xcb_window_t w = e->pad[2];

        switch (e->response_type & ~0x80)
        {
            case XCB_EXPOSE:
            {
                auto ev = (xcb_expose_event_t*)e;
                w = ev->window;
            }
            break;
            case XCB_CONFIGURE_NOTIFY:
            case XCB_PROPERTY_NOTIFY:
            case XCB_CLIENT_MESSAGE:
            {
                auto ev = (xcb_configure_notify_event_t*)&e;
                w = e->pad[0];
            }
            break;
            default: break;
        }

        auto wnd = windows.find(w);
        if (wnd != windows.end())
        {
            auto &wnd_data = wnd->second;
            if (!wnd_data.created)
            {
                wnd_data.created = true;
                event ev;
                ev.type = wui::event_type::internal;
                ev.internal_event_.type = wui::internal_event_type::window_created;
                wnd_data.window_->receive_control_events(ev);
            }
            wnd_data.window_->process_events(*e);
        }

        free(e);
    }
    return processed;
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
    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(timer_mutex_);
        removed = timers_.erase(id) > 0;
    }
    if (removed) wake_timer_thread();
    return removed;
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

        // Reschedule relative to the current instant (drift-free and without
        // the double-interval case of "reconfiguring inside the callback"):
        // if the callback cancelled this timer it no longer exists and is not
        // rescheduled; otherwise the next tick is one interval away.
        std::lock_guard<std::mutex> lock(timer_mutex_);
        auto it = timers_.find(id);
        if (it != timers_.end())
        {
            it->second.next = std::chrono::steady_clock::now() + it->second.interval;
        }
    }
}

listener& get_listener()
{
    static listener instance;
    return instance;
}

}
