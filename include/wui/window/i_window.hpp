//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/control/i_control.hpp>
#include <wui/theme/i_theme.hpp>
#include <wui/event/event.hpp>
#include <wui/common/error.hpp>

#include <functional>

#include <string>

namespace wui
{

enum class window_style : uint32_t
{
    resizable = (1 << 0),
    moving = (1 << 1),

    close_button = (1 << 2),
    expand_button = (1 << 3),
    minimize_button = (1 << 4),
    pin_button = (1 << 5),
    switch_theme_button = (1 << 6),
	switch_lang_button = (1 << 7),

    title_showed = (1 << 10),
    topmost = (1 << 11),

    border_left = (1 << 15),
    border_top = (1 << 16),
    border_right = (1 << 17),
    border_bottom = (1 << 18),
    border_all = border_left | border_top | border_right | border_bottom,

    frame = title_showed | close_button | expand_button | minimize_button | resizable | moving,
    dialog = title_showed | close_button | moving | border_all,
    pinned = pin_button | close_button | resizable | moving
};

struct system_context;

class i_window
{
public:
    virtual bool init(std::string_view caption, rect position, window_style style, std::function<void(void)> close_callback) = 0;
    virtual void destroy() = 0;

    virtual void add_control(std::shared_ptr<i_control> control, rect position) = 0;
    virtual void remove_control(std::shared_ptr<i_control> control) = 0;
    
    virtual void bring_to_front(std::shared_ptr<i_control> control) = 0;
    virtual void move_to_back(std::shared_ptr<i_control> control) = 0;

    virtual void redraw(rect position, bool clear = false) = 0;

    virtual std::string subscribe(std::function<void(const event&)> receive_callback, event_type event_types, std::shared_ptr<i_control> control = nullptr) = 0;
    virtual void unsubscribe(std::string_view subscriber_id) = 0;

    virtual system_context &context() = 0;

    virtual error get_error() const = 0;

protected:
    ~i_window() {}

};

}
