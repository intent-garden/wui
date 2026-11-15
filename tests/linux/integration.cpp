#include "../common/list_layout.hpp"
// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#include <wui/control/text.hpp>
#include <wui/framework/framework.hpp>
#include <wui/system/timer.hpp>
#include <wui/theme/theme.hpp>
#include <wui/window/listener.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
#define CHECK(x) do { if (!(x)) throw std::runtime_error("line " + std::to_string(__LINE__) + ": " #x); } while (false)

template<class F> auto on_ui(F callback)
{
    using result_type = decltype(callback());
    auto task = std::make_shared<std::packaged_task<result_type()>>(std::move(callback));
    auto result = task->get_future();
    wui::get_listener().post([task] { (*task)(); });
    CHECK(result.wait_for(5s) == std::future_status::ready);
    return result.get();
}

template<class F> void until(F condition)
{
    auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!condition())
    {
        CHECK(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(5ms);
    }
}

class painted_text : public wui::text
{
public:
    int paints = 0;
    std::thread::id draw_thread;
    wui::rect last_area{};
    void draw(wui::graphic& graphic, wui::rect area) override
    {
        ++paints;
        draw_thread = std::this_thread::get_id();
        last_area = area;
        text::draw(graphic, area);
    }
};

// Run from the test thread so the listener can process real X11 events between
// assertions. No test-only access to window's private event dispatcher is used.
void linux_list_hit_tests()
{
    std::shared_ptr<wui::window> view;
    std::shared_ptr<wui::list> rows;
    int calls = 0, measured = 0, delivered = 0, selected = -1;
    std::string subscription;
    on_ui([&] {
        view = std::make_shared<wui::window>();
        rows = std::make_shared<wui::list>();
        view->add_control(rows, {20, 50, 320, 250});
        rows->set_item_height_callback([&](int row, int& height) {
            CHECK(row >= 0 && row < 2000);
            ++calls;
            height = 20;
        });
        rows->set_item_count(2000);
        rows->scroll_to_end();
        subscription = view->subscribe([&](const wui::event& ev) {
            if (ev.internal_event_.type == wui::internal_event_type::user_emitted)
            {
                measured = calls;
                selected = rows->selected_item();
                ++delivered;
            }
        }, wui::event_type::internal);
        CHECK(view->init("Native list hit testing", {0, 0, 400, 320}, wui::window_style::frame));
    });
    auto send = [&](bool click) {
        const int expected = on_ui([&] {
            calls = 0;
            auto& context = view->context();
            xcb_motion_notify_event_t move{};
            move.response_type = XCB_MOTION_NOTIFY;
            move.event = context.wnd;
            move.event_x = 30;
            move.event_y = 235;
            xcb_send_event(context.connection, false, context.wnd, XCB_EVENT_MASK_POINTER_MOTION,
                reinterpret_cast<const char*>(&move));
            if (click)
            {
                xcb_button_press_event_t button{};
                button.response_type = XCB_BUTTON_PRESS;
                button.detail = XCB_BUTTON_INDEX_1;
                button.time = 1000;
                button.event = context.wnd;
                button.event_x = 30;
                button.event_y = 235;
                xcb_send_event(context.connection, false, context.wnd, XCB_EVENT_MASK_BUTTON_PRESS,
                    reinterpret_cast<const char*>(&button));
                button.response_type = XCB_BUTTON_RELEASE;
                xcb_send_event(context.connection, false, context.wnd, XCB_EVENT_MASK_BUTTON_RELEASE,
                    reinterpret_cast<const char*>(&button));
            }
            view->emit_event(42, 0);
            auto reply = xcb_get_input_focus_reply(context.connection, xcb_get_input_focus(context.connection), nullptr);
            free(reply);
            return delivered + 1;
        });
        until([&] { return on_ui([&] { return delivered >= expected; }); });
    };
    send(false);
    on_ui([&] { CHECK(measured <= 2000); });
    send(true);
    on_ui([&] {
        CHECK(measured <= 8000); // One hover, one hit test and selected-row damage.
        CHECK(selected == 1999);
        view->unsubscribe(subscription);
        rows->set_item_height_callback({});
        view->destroy();
        view->remove_control(rows);
    });
    std::puts("PASS: Linux long-list hover and selection through real X11 events");
}

int main()
{
    try
    {
        auto& listener = wui::get_listener();
        CHECK(listener.init());
        CHECK(listener.init()); // Idempotent: no leaked display or replaced thread.
        auto ui_thread = on_ui([] { return std::this_thread::get_id(); });
        CHECK(ui_thread != std::this_thread::get_id());
        std::shared_ptr<wui::window> window;
        std::shared_ptr<painted_text> label;
        int created = 0, resized = 0, closed = 0, emitted = 0, width_at_input = 0;
        bool reopen_on_close = false;
        on_ui([&] {
            CHECK(wui::set_default_theme_from_file("dark", WUI_SOURCE_DIR "/examples/simple/res/dark.json"));
            CHECK(run_list_layout_tests());
            window = std::make_shared<wui::window>();
            label = std::make_shared<painted_text>();
            window->add_control(label, {0, 0, 250, 200});
            window->subscribe([&](const wui::event& ev) {
                if (ev.internal_event_.type == wui::internal_event_type::window_created)
                {
                    ++created;
                    CHECK(window->get_graphic().drawable());
                }
                if (ev.internal_event_.type == wui::internal_event_type::size_changed) ++resized;
                if (ev.internal_event_.type == wui::internal_event_type::user_emitted)
                {
                    ++emitted;
                    width_at_input = window->position().width();
                }
            }, wui::event_type::internal);
            CHECK(window->init("Linux UI test", {0, 0, 480, 320}, wui::window_style::frame, [&] {
                ++closed;
                if (reopen_on_close)
                {
                    reopen_on_close = false;
                    window = std::make_shared<wui::window>();
                    CHECK(window->init("Reopened in close callback", {0, 0, 320, 240},
                        wui::window_style::frame, [&] { ++closed; }));
                }
            }));
        });
        linux_list_hit_tests();
        until([&] { return on_ui([&] { return label->paints > 0; }); });
        on_ui([&] {
            CHECK(created == 1); // No duplicate window_created on the first X event.
            CHECK(label->draw_thread == ui_thread);
        });

        // Many independent timers share the event/paint thread.
        std::atomic<int> ticks{0};
        std::atomic<bool> wrong_thread{false};
        std::vector<std::unique_ptr<wui::timer>> timers;
        for (int i = 0; i < 32; ++i)
        {
            timers.emplace_back(new wui::timer([&] {
                if (std::this_thread::get_id() != ui_thread) wrong_thread = true;
                ++ticks;
            }));
            timers.back()->start(5);
        }
        until([&] { return ticks.load() >= 64; });
        for (auto& timer : timers) timer->stop();
        CHECK(!wrong_thread);
        const auto stopped_ticks = ticks.load();
        std::this_thread::sleep_for(30ms);
        CHECK(ticks == stopped_ticks);
        CHECK(listener.timer_count() == 0);
        timers.clear();

        // Self-destruction and restart cannot join the current event thread or
        // allow a stale copied callback to run for a newly started generation.
        std::unique_ptr<wui::timer> self;
        std::atomic<int> self_ticks{0};
        on_ui([&] {
            self.reset(new wui::timer([&] { ++self_ticks; self.reset(); }));
            self->start(1);
        });
        until([&] { return self_ticks.load() == 1; });
        on_ui([&] { CHECK(!self); });
        CHECK(listener.timer_count() == 0);
        wui::timer* restarting_ptr = nullptr;
        std::atomic<int> restarts{0};
        wui::timer restarting([&] {
            restarting_ptr->stop();
            if (++restarts == 1) restarting_ptr->start(1);
        });
        restarting_ptr = &restarting;
        restarting.start(1);
        until([&] { return restarts.load() == 2; });
        restarting.stop();
        CHECK(listener.timer_count() == 0);

        // stop() from another thread waits for an in-flight callable to finish.
        std::promise<void> entered, release;
        auto released = release.get_future().share();
        bool first_entry = true;
        wui::timer blocked([&] { if (first_entry) { first_entry = false; entered.set_value(); } released.wait(); });
        auto entering = entered.get_future();
        blocked.start(1);
        CHECK(entering.wait_for(5s) == std::future_status::ready);
        auto stopping = std::async(std::launch::async, [&] { blocked.stop(); });
        const auto stop_status = stopping.wait_for(30ms);
        release.set_value();
        CHECK(stop_status == std::future_status::timeout);
        CHECK(stopping.wait_for(5s) == std::future_status::ready);
        stopping.get();
        CHECK(listener.timer_count() == 0);

        // A redraw storm produces a bounded number of paints. The union must
        // cover both requested areas, including the one requesting a clear.
        int before = on_ui([&] {
            auto count = label->paints;
            for (int i = 0; i < 10000; ++i)
                window->redraw(i % 2 ? wui::rect{0, 0, 30, 30} : wui::rect{100, 100, 200, 180}, i == 0);
            return count;
        });
        until([&] { return on_ui([&] { return label->paints > before; }); });
        on_ui([&] {
            CHECK(label->paints - before <= 3);
            CHECK(label->last_area.left == 0 && label->last_area.top == 0);
            CHECK(label->last_area.right >= 200 && label->last_area.bottom >= 180);
            std::printf("Redraw storm: 10000 requests -> %d paints\n", label->paints - before);
        });

        // Queue real X11 events on a single UI turn. event != window exercises
        // ConfigureNotify routing; input following the burst must see its final
        // geometry, while resize callbacks are bounded by the batch size.
        int before_resize = on_ui([&] {
            const int count = resized;
            auto& context = window->context();
            for (int i = 0; i < 1000; ++i)
            {
                xcb_configure_notify_event_t ev{};
                ev.response_type = XCB_CONFIGURE_NOTIFY;
                ev.event = context.screen->root;
                ev.window = context.wnd;
                ev.width = 300 + i % 100;
                ev.height = 220;
                xcb_send_event(context.connection, false, context.wnd, XCB_EVENT_MASK_STRUCTURE_NOTIFY,
                    reinterpret_cast<const char*>(&ev));
            }
            window->emit_event(42, 7);
            auto reply = xcb_get_input_focus_reply(context.connection, xcb_get_input_focus(context.connection), nullptr);
            free(reply); // Round trip also moves all queued events into XCB.
            return count;
        });
        until([&] { return on_ui([&] { return emitted == 1; }); });
        on_ui([&] {
            CHECK(width_at_input == 399);
            CHECK(resized > before_resize && resized - before_resize < 40);
            std::printf("Resize storm: 1000 events -> %d layout callbacks\n", resized - before_resize);
        });

        // Close the last window, then create another INSIDE its close callback.
        // The registry must keep the old target alive without holding its lock.
        on_ui([&] {
            reopen_on_close = true;
            window->destroy();
        });
        until([&] { return on_ui([&] { return closed == 1; }); });
        on_ui([&] {
            CHECK(window->context().wnd != 0);
            label.reset();
            window->destroy();
        });
        until([&] { return on_ui([&] { return closed == 2; }); });
        // Timers remain usable even while there are no windows in the registry.
        std::atomic<int> idle_ticks{0};
        wui::timer idle([&] { ++idle_ticks; });
        idle.start(1);
        until([&] { return idle_ticks.load() >= 2; });
        idle.stop();
        on_ui([&] {
            window.reset();
            window = std::make_shared<wui::window>();
            CHECK(window->init("Reopened from task", {0, 0, 320, 240},
                wui::window_style::frame, [&] { ++closed; }));
            window->destroy();
        });
        until([&] { return on_ui([&] { return closed == 3; }); });
        on_ui([&] { window.reset(); });
        // An idle listener must wake and join promptly at destruction, without
        // waiting for another X event. CTest bounds this test's total runtime.
        {
            wui::listener isolated;
            CHECK(isolated.init());
            std::promise<void> running;
            auto ready = running.get_future();
            isolated.post([&] { running.set_value(); });
            CHECK(ready.wait_for(5s) == std::future_status::ready);
        }
        std::puts("PASS: Linux UI timer lifecycle, redraw batching, resize/input ordering and window reopening");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
