// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#include <wui/system/timer.hpp>
#include <wui/framework/framework.hpp>
#include <wui/theme/theme.hpp>
#include <wui/window/window.hpp>
#include <wui/control/input.hpp>
#include <wui/control/button.hpp>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
#define CHECK(x) do { if (!(x)) throw std::runtime_error("line " + std::to_string(__LINE__) + ": " #x); } while (false)

void pump()
{
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

template<class F> void until(F condition)
{
    auto end = std::chrono::steady_clock::now() + 5s;
    while (!condition())
    {
        CHECK(std::chrono::steady_clock::now() < end);
        pump();
        MsgWaitForMultipleObjectsEx(0, nullptr, 2, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
}

void pump_for(std::chrono::milliseconds duration)
{
    auto end = std::chrono::steady_clock::now() + duration;
    until([&] { return std::chrono::steady_clock::now() >= end; });
}

int main()
{
    try
    {
        wui::framework::init();
        const auto owner = GetCurrentThreadId();
        std::atomic<int> ticks{0};
        std::atomic<bool> wrong_thread{false};
        std::vector<std::unique_ptr<wui::timer>> timers;
        for (int i = 0; i < 16; ++i)
        {
            timers.emplace_back(new wui::timer([&] {
                if (GetCurrentThreadId() != owner) wrong_thread = true;
                ++ticks;
            }));
            timers.back()->start(10);
        }
        until([&] { return ticks.load() >= 32; });
        for (auto& t : timers) t->stop();
        int stopped = ticks.load();
        pump_for(30ms);
        CHECK(ticks == stopped && !wrong_thread);
        timers.clear();

        std::unique_ptr<wui::timer> self;
        int self_ticks = 0;
        self.reset(new wui::timer([&] { ++self_ticks; self.reset(); }));
        self->start(10);
        until([&] { return self_ticks == 1; });
        CHECK(!self);
        wui::timer* restarting_ptr = nullptr;
        int restarts = 0;
        wui::timer restarting([&] {
            restarting_ptr->stop();
            if (++restarts == 1) restarting_ptr->start(10);
        });
        restarting_ptr = &restarting;
        restarting.start(10);
        until([&] { return restarts == 2; });
        restarting.stop();

        // KillTimer leaves previously posted WM_TIMER messages in the queue.
        // A stale message must not call a restarted timer's new generation.
        int stale_ticks = 0;
        wui::timer stale([&] { ++stale_ticks; });
        stale.start(10);
        MSG pending{};
        auto deadline = std::chrono::steady_clock::now() + 5s;
        while (!PeekMessageW(&pending, nullptr, WM_TIMER, WM_TIMER, PM_REMOVE))
        {
            CHECK(std::chrono::steady_clock::now() < deadline);
            Sleep(1);
        }
        stale.stop();
        stale.start(1000);
        DispatchMessageW(&pending);
        CHECK(stale_ticks == 0);
        stale.stop();

        // Nested modal/message loops must not re-enter a callback.
        int depth = 0, max_depth = 0, nested_ticks = 0;
        wui::timer* nested_ptr = nullptr;
        wui::timer nested([&] {
            ++depth;
            max_depth = (std::max)(max_depth, depth);
            ++nested_ticks;
            pump_for(35ms);
            nested_ptr->stop();
            --depth;
        });
        nested_ptr = &nested;
        nested.start(10);
        until([&] { return nested_ticks != 0; });
        CHECK(nested_ticks == 1 && max_depth == 1);

        // Repeated start/stop from multiple workers previously raced deletion
        // of the same timer queue. No callback may move onto those workers.
        std::atomic<int> finished{0};
        wui::timer raced([&] { if (GetCurrentThreadId() != owner) wrong_thread = true; ++ticks; });
        auto race = [&] {
            for (int i = 0; i < 2000; ++i) { raced.start(10); raced.stop(); raced.stop(); }
            ++finished;
        };
        std::thread one(race), two(race);
        until([&] { return finished.load() == 2; });
        one.join(); two.join();
        raced.stop();
        CHECK(!wrong_thread);

        std::promise<void> entered;
        auto entering = entered.get_future();
        std::atomic<bool> callback_done{false}, cancellation_done{false}, stopped_too_early{false};
        bool first = true;
        wui::timer running([&] {
            if (first) { first = false; entered.set_value(); }
            Sleep(30);
            callback_done = true;
        });
        std::thread canceller([&] {
            entering.wait();
            running.stop();
            stopped_too_early = !callback_done.load();
            cancellation_done = true;
        });
        running.start(10);
        until([&] { return cancellation_done.load(); });
        canceller.join();
        CHECK(!stopped_too_early);

        // Queued destruction on the owner thread must release USER resources.
        pump();
        const DWORD before = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
        for (int i = 0; i < 300; ++i)
        {
            auto timer = std::make_unique<wui::timer>([] {});
            timer->start(10);
            std::thread destroyer([owned = std::move(timer)]() mutable { owned.reset(); });
            destroyer.join();
            pump();
        }
        CHECK(GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS) <= before + 1);

        // Exercise the same mouse/focus/scroll timer cancellation path as the
        // simple example, including animation and repeated resizes.
        CHECK(wui::set_default_theme_from_file("dark", WUI_SOURCE_DIR "/examples/simple/res/dark.json"));
        auto window = std::make_shared<wui::window>();
        std::string content;
        for (int i = 0; i < 50; ++i) content += "Line " + std::to_string(i) + "\n";
        auto input = std::make_shared<wui::input>(content, wui::input_view::multiline);
        auto button = std::make_shared<wui::button>("Focus target", [] {});
        window->add_control(input, {20, 60, 280, 170});
        window->add_control(button, {20, 200, 220, 235});
        CHECK(window->init("WUI Windows integration", {0, 0, 480, 320}, wui::window_style::frame));
        auto hwnd = window->context().hwnd;
        int heartbeat_ticks = 0;
        wui::timer heartbeat([&] { ++heartbeat_ticks; });
        heartbeat.start(10);
        for (int i = 0; i < 400; ++i)
        {
            int y = i % 2 ? 215 : 85;
            SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(35, y));
            SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(35, y));
            SendMessageW(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(35, y));
            if (i % 20 == 0)
            {
                SendMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(277, 95));
                pump_for(25ms); // Exercise a real scrollbar animation tick.
            }
            if (i % 40 == 0) window->set_position({0, 0, 480 + i % 80, 320});
            pump_for(1ms);
        }
        heartbeat.stop();
        CHECK(heartbeat_ticks > 1);
        window->destroy();
        pump();
        std::puts("PASS: Windows UI timers, concurrent cancellation, stale messages, self-destruction, resources and mouse/focus stress");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
