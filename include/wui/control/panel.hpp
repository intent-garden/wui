//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <wui/control/i_control.hpp>
#include <wui/graphic/graphic.hpp>
#include <wui/common/rect.hpp>
#include <wui/common/color.hpp>

#include <string>
#include <functional>
#include <memory>

namespace wui
{

class panel : public i_control, public std::enable_shared_from_this<panel>
{
public:
    panel(std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    panel(std::function<void(graphic&)> draw_callback, std::string_view theme_control_name = tc, std::shared_ptr<i_theme> theme_ = nullptr);
    ~panel();

    virtual void draw(graphic &gr, rect );

    virtual void set_position(rect position);
    virtual rect position() const;

    virtual void set_parent(std::shared_ptr<window> window_);
    virtual std::weak_ptr<window> parent() const;
    virtual void clear_parent();

    virtual void set_topmost(bool yes);
    virtual bool topmost() const;

    virtual void update_theme_control_name(std::string_view theme_control_name);
    virtual void update_theme(std::shared_ptr<i_theme> theme_ = nullptr);

    virtual void show();
    virtual void hide();
    virtual bool showed() const;

    virtual void enable();
    virtual void disable();
    virtual bool enabled() const;

    virtual bool focused() const;
    virtual bool focusing() const;

    virtual error get_error() const;

public:
    /// Control name in theme
    static constexpr const char *tc = "panel";

    /// Used theme values
    static constexpr const char *tv_background = "background";

private:
    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    rect position_;

    std::weak_ptr<window> parent_;

    bool showed_, topmost_;

    std::function<void(graphic&)> draw_callback;

    void redraw();
};

}
