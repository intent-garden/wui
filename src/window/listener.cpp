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
#include <chrono>
#include <functional>
#include <poll.h>


namespace wui
{

listener::listener()
    : started(false),
    thread{},
    context_{},
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

    start();

    return true;
}

void listener::start()
{
    if (started)
    {
        return;
    }

    started = true;
    thread = std::thread(std::bind(&listener::process_events, this));
}

void listener::stop()
{
    started = false;
    if (thread.joinable()) thread.join();

    XCloseDisplay(context_.display);
}

system_context const &listener::context() const
{
    return context_;
}

void listener::process_events()
{
    const int fd = xcb_get_file_descriptor(context_.connection);
    xcb_flush(context_.connection);
    while (started)
    {
        const auto now = std::chrono::steady_clock::now();

        // Poll the X connection so the loop stays responsive to both X
        // events and the periodic timer callback (same listener thread).
        int timeout_ms = 100;
        if (timer_enabled_)
        {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                next_tick_ - now).count();
            timeout_ms = static_cast<int>(std::clamp<long long>(remaining, 1, 100));
        }

        struct pollfd pfd {};
        pfd.fd = fd;
        pfd.events = POLLIN;
        const int rc = poll(&pfd, 1, timeout_ms);

        if (rc > 0 && (pfd.revents & POLLIN))
        {
            xcb_generic_event_t *e = nullptr;
            while ((e = xcb_poll_for_event(context_.connection)))
            {
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
                    auto &w = wnd->second;
                    if (!w.created)
                    {
                        w.created = true;
                        event ev;
                        ev.type = wui::event_type::internal;
                        ev.internal_event_.type = wui::internal_event_type::window_created;
                        w.window_->receive_control_events(ev);
                    }
                    w.window_->process_events(*e);
                }

                free(e);
            }
        }

        dispatch_due_timer();
    }
}

void listener::set_timer(std::chrono::milliseconds interval,
                         std::function<void()> callback)
{
    timer_interval_ = interval > std::chrono::milliseconds(0)
        ? interval : std::chrono::milliseconds(16);
    on_tick_ = std::move(callback);
    next_tick_ = std::chrono::steady_clock::now() + timer_interval_;
    timer_enabled_ = static_cast<bool>(on_tick_);
}

void listener::clear_timer()
{
    timer_enabled_ = false;
    on_tick_ = {};
}

void listener::dispatch_due_timer()
{
    if (!timer_enabled_ || !on_tick_) return;
    const auto now = std::chrono::steady_clock::now();
    if (now < next_tick_) return;
    auto callback = on_tick_;
    callback();
    const auto next = next_tick_ + timer_interval_;
    next_tick_ = (next > now) ? next : now + timer_interval_;
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
