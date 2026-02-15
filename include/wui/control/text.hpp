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
#include <wui/common/alignment.hpp>

#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace wui
{

class text : public i_control, public std::enable_shared_from_this<text>
{
public:
    text(std::string_view text = "",
        hori_alignment hori_alignment_ = hori_alignment::left,
        vert_alignment vert_alignment_ = vert_alignment::center,
        std::string_view theme_control_name = tc,
        std::shared_ptr<i_theme> theme_ = nullptr);
    ~text();

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
    /// Text's interface
    void set_text(std::string_view text);
    std::string_view get_text() const;

    void set_alignment(hori_alignment hori_alignment_, vert_alignment vert_alignment_);

public:
    /// Control name in theme
    static constexpr const char *tc = "text";

    /// Used theme values
    static constexpr const char *tv_color = "color";
    static constexpr const char *tv_font = "font";

private:
    std::string tcn; /// control name in theme
    std::shared_ptr<i_theme> theme_;

    rect position_;

    std::weak_ptr<window> parent_;

    bool showed_, topmost_;

    std::string text_;

    hori_alignment hori_alignment_;
    vert_alignment vert_alignment_;
    
    void redraw();
};

}
