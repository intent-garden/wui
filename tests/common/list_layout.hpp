// Copyright (c) 2026 Intent Garden Org. Boost Software License 1.0.
#pragma once
#include <wui/control/list.hpp>
#include <wui/window/window.hpp>
#include <cstdio>
#include <stdexcept>
#include <vector>
#ifdef __APPLE__
#include "../../src/macos/window_mac.hpp"
#elif __EMSCRIPTEN__
#include "../../src/wasm/window_wasm.hpp"
#endif

// Invoke this on the platform event thread, with a default theme installed.
// Measure callback counts, not wall time, so slow CI cannot hide quadratic work.
inline bool run_list_layout_tests()
{
    using namespace wui;
    auto check = [](bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
    };
    auto window = std::make_shared<wui::window>();
    auto field = std::make_shared<list>();
    window->add_control(field, {20, 50, 320, 250});
    check(window->init("List layout tests", {0, 0, 400, 320}, window_style::frame), "list window init");
    int calls = 0, invalid = 0;
    constexpr int rows = 2000;
    field->set_item_height_callback([&](int row, int& height) {
        ++calls;
        if (row < 0 || row >= rows) { ++invalid; return; }
        height = 20;
    });
    field->set_item_count(rows);
    field->set_mode(list::list_mode::simple);
    field->scroll_to_end();
    std::vector<int> painted;
    int active = -1;
    field->set_draw_callback([&](graphic&, int row, rect area, list::item_state state) {
        painted.push_back(row);
        check(row >= 0 && row < rows, "draw requested an invalid row");
        check(area.bottom > 0 && area.top < 200, "draw requested an invisible row");
        if (state == list::item_state::active) active = row;
    });
    calls = 0;
    field->draw(window->get_graphic(), {});
    check(calls <= rows, "drawing a long list repeats prefix height measurement");
    check(!painted.empty() && painted.back() == rows - 1, "last history row is missing");
    check(painted.size() <= 11, "drawing walks invisible rows as draw callbacks");
    std::printf("List history: %d rows, %d height calls per paint, %zu visible rows\n", rows, calls, painted.size());

    uint32_t mouse_time = 0;
    auto mouse = [&](mouse_event_type type, int y) {
        event ev{};
        ev.type = event_type::mouse;
        ev.mouse_event_.type = type;
        ev.mouse_event_.x = 30;
        ev.mouse_event_.y = y;
        // Same control subscription used by platform window event routing.
#ifdef __APPLE__
        macos_window_backend::mouse(*window, ev.mouse_event_);
#elif __EMSCRIPTEN__
        wasm_window_backend::mouse(*window, ev.mouse_event_);
#elif _WIN32
        const UINT message = type == mouse_event_type::move ? WM_MOUSEMOVE :
            type == mouse_event_type::left_down ? WM_LBUTTONDOWN : WM_LBUTTONUP;
        SendMessageW(window->context().hwnd, message, type == mouse_event_type::left_down ? MK_LBUTTON : 0,
            MAKELPARAM(30, y));
#else
        xcb_motion_notify_event_t native{};
        native.response_type = type == mouse_event_type::move ? XCB_MOTION_NOTIFY :
            type == mouse_event_type::left_down ? XCB_BUTTON_PRESS : XCB_BUTTON_RELEASE;
        native.detail = XCB_BUTTON_INDEX_1;
        native.event = window->context().wnd;
        native.event_x = 30;
        native.event_y = y;
        native.time = (mouse_time += 500);
        window->process_events(reinterpret_cast<xcb_generic_event_t&>(native));
#endif
    };
    mouse(mouse_event_type::move, 235);
    calls = 0;
    mouse(mouse_event_type::move, 235);
    check(calls <= rows, "hovering a long list repeats prefix height measurement");
    active = -1;
    field->draw(window->get_graphic(), {});
    check(active == rows - 1, "hover does not identify the last visible row");
    calls = 0;
    mouse(mouse_event_type::left_down, 235);
    mouse(mouse_event_type::left_up, 235);
    check(calls <= rows * 3, "selecting a row repeats prefix height measurement");
    check(field->selected_item() == rows - 1, "click did not select the last row");
    check(invalid == 0, "height callback received an out-of-range index");

    // The backing data can temporarily disappear while switching a server or
    // a chat. Missing and zero-height rows must terminate and never be drawn.
    field->set_item_height_callback([&](int, int& height) { ++calls; height = -1; });
    field->set_item_count(rows);
    painted.clear(); calls = 0;
    field->draw(window->get_graphic(), {});
    check(calls <= rows && painted.empty(), "missing rows cause excessive work or drawing");
    field->set_item_height_callback([](int row, int& height) { height = row == 1 ? 0 : row == 2 ? -1 : row == 3 ? -5 : 20; });
    field->set_item_count(5);
    check(field->get_item_top(4) == 20 && field->get_item_top(5) == 40, "nonpositive heights shift rows backwards");
    field->scroll_to_start();
    mouse(mouse_event_type::left_down, 75);
    mouse(mouse_event_type::left_up, 75);
    check(field->selected_item() == 4, "variable-height hit testing is wrong");
    field->set_item_count(-1);
    check(field->get_item_count() == 0, "negative count was not clamped");
    field->set_draw_callback({});
    field->set_item_height_callback({});
    window->destroy();
    window->remove_control(field);
    std::puts("PASS: linear list painting/hit testing and unavailable row handling");
    return true;
}
