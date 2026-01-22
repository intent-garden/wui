//
// Copyright (c) 2021-2025 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/control/button.hpp>

#include <wui/window/window.hpp>

#include <wui/control/image.hpp>
#include <wui/control/tooltip.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>
#include <wui/common/flag_helpers.hpp>

namespace wui
{

button::button(std::string_view caption_, std::function<void(void)> click_callback_, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : button_view_(button_view::text),
    caption(caption_),
    image_(),
    image_size(0),
    tooltip_(std::make_shared<tooltip>(caption_, tooltip::tc, theme__)),
    click_callback(click_callback_),
    tcn(theme_control_name_),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    my_subscriber_id(),
    showed_(true), enabled_(true), topmost_(false), active(false), focused_(false),
    focusing_(theme_dimension(tcn, tv_focusing, theme_) != 0),
    pushed(false),
    turned_(false),
    text_rect{ 0 },
    err{}
{
}

std::shared_ptr<image> get_button_image(button_view button_view_, std::shared_ptr<i_theme> theme_)
{
    switch (button_view_)
    {
        case button_view::switcher:
            return std::make_shared<image>(theme_image(button::ti_switcher_on, theme_));
        break;
        case button_view::radio:
            return std::make_shared<image>(theme_image(button::ti_radio_on, theme_));
        break;
        default:
            return nullptr;
        break;
    }
}

button::button(std::string_view caption_, std::function<void(void)> click_callback_, button_view button_view__, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : button_view_(button_view__),
    caption(caption_),
    image_(get_button_image(button_view__, theme__)),
    image_size(0),
    tooltip_(std::make_shared<tooltip>(caption_, tooltip::tc, theme__)),
    click_callback(click_callback_),
    tcn(theme_control_name_),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    my_subscriber_id(),
    showed_(true), enabled_(true), topmost_(false), active(false), focused_(false),
    focusing_(theme_dimension(tcn, tv_focusing, theme_) != 0),
    pushed(false),
    turned_(false),
    text_rect{ 0 },
    err{}
{
    if (image_) update_err("button::constructor[image from theme standart buttons]", image_->get_error());
}

#ifdef _WIN32
button::button(std::string_view caption_, std::function<void(void)> click_callback_, button_view button_view__, int32_t image_resource_index_, int32_t image_size_, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : button_view_(button_view__),
    caption(caption_),
    image_(std::make_shared<image>(image_resource_index_, theme__)),
    image_size(image_size_),
    tooltip_(std::make_shared<tooltip>(caption_, tooltip::tc, theme__)),
    click_callback(click_callback_),
    tcn(theme_control_name_),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    showed_(true), enabled_(true), topmost_(false), active(false), focused_(false),
    focusing_(theme_dimension(tcn, tv_focusing, theme_) != 0),
    pushed(false),
    turned_(false),
    text_rect{ 0 },
    err{}
{
    update_err("button::constructor[image from resource]", image_->get_error());
}
#endif

button::button(std::string_view caption_, std::function<void(void)> click_callback_, button_view button_view__, std::string_view imageFileName_, int32_t image_size_, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : button_view_(button_view__),
    caption(caption_),
    image_(std::make_shared<image>(imageFileName_, theme__)),
    image_size(image_size_),
    tooltip_(std::make_shared<tooltip>(caption_, tooltip::tc, theme__)),
    click_callback(click_callback_),
    tcn(theme_control_name_),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    showed_(true), enabled_(true), topmost_(false), active(false), focused_(false),
    focusing_(theme_dimension(tcn, tv_focusing, theme_) != 0),
    pushed(false),
    turned_(false),
    text_rect{ 0 },
    err{}
{
    update_err("button::constructor[image from file]", image_->get_error());
}

button::button(std::string_view caption_, std::function<void(void)> click_callback_, button_view button_view__, const std::vector<uint8_t> &image_data, int32_t image_size_, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : button_view_(button_view__),
    caption(caption_),
    image_(std::make_shared<image>(image_data)),
    image_size(image_size_),
    tooltip_(std::make_shared<tooltip>(caption_, tooltip::tc, theme__)),
    click_callback(click_callback_),
    tcn(theme_control_name_),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    my_subscriber_id(),
    showed_(true), enabled_(true), topmost_(false), active(false), focused_(false),
    focusing_(theme_dimension(tcn, tv_focusing, theme_) != 0),
    pushed(false),
    turned_(false),
    text_rect{ 0 },
    err{}
{
    update_err("button::constructor[image from data]", image_->get_error());
}

button::~button()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

void button::draw(graphic &gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    auto font_ = theme_font(tcn, tv_font, theme_);

    if (button_view_ != button_view::image && !caption.empty() && text_rect.width() == 0)
    {
        text_rect = measure_text(caption, font_, &gr);
        text_rect.bottom += 2;
    }

    int32_t text_top = 0, text_left = 0, image_left = 0, image_top = 0;

    auto control_pos = position();

    switch (button_view_)
    {
        case button_view::text: case button_view::anchor: case button_view::sheet:
            if (text_rect.right + 10 > position_.width())
            {
                position_.right = position_.left + text_rect.right + 10;
                return redraw();
            }
            if (text_rect.bottom + 6 > position_.height())
            {
                position_.bottom = position_.top + text_rect.bottom + 6;
                return redraw();
            }

            text_left = button_view_ == button_view::text ? control_pos.left + ((control_pos.width() - text_rect.right) / 2) : control_pos.left;
            text_top = control_pos.top + ((control_pos.height() - text_rect.bottom) / 2);
        break;
        case button_view::image:
            if (image_)
            {
                if (image_size > position_.width())
                {
                    position_.right = position_.left + image_size;
                    return redraw();
                }
                if (image_size > position_.height())
                {
                    position_.bottom = position_.top + image_size;
                    return redraw();
                }

                image_left = control_pos.left + ((control_pos.width() - image_size) / 2);
                image_top = control_pos.top + ((control_pos.height() - image_size) / 2);
            }
        break;
        case button_view::image_right_text:
            if (image_)
            {
                if (image_size + text_rect.right + 10 > position_.width())
                {
                    position_.right = position_.left + text_rect.right + image_size + 10;
                    return redraw();
                }
                if (image_size + 10 > position_.height())
                {
                    position_.bottom = position_.top + image_size + 10;
                    return redraw();
                }
                if (text_rect.bottom + 6 > position_.height())
                {
                    position_.bottom = position_.top + text_rect.bottom + 6;
                    return redraw();
                }

                image_left = control_pos.left + ((control_pos.width() - text_rect.right - image_size - 5) / 2);
                image_top = control_pos.top + ((control_pos.height() - image_size) / 2);
                text_left = image_left + image_size + 5;
                text_top = control_pos.top + ((control_pos.height() - text_rect.bottom) / 2);
            }
        break;
        case button_view::switcher: case button_view::radio:
            if (image_)
            {
                if (image_->height() + 6 > position_.height())
                {
                    position_.bottom = position_.top + image_->height() + 6;
                    return redraw();
                }
                if (text_rect.bottom + 6 > position_.height())
                {
                    position_.bottom = position_.top + text_rect.bottom + 6;
                    return redraw();
                }

                image_left = control_pos.left;
                image_top = control_pos.top + ((control_pos.height() - image_->height()) / 2);
                text_left = image_left + image_->width() + 5;
                text_top = control_pos.top + ((control_pos.height() - text_rect.bottom) / 2);

                truncate_line(caption, gr, font_, control_pos.right - text_left - 10);
            }
        break;
        case button_view::image_bottom_text:
            if (image_)
            {
                if (image_size + 10 > position_.width())
                {
                    position_.right = position_.left + image_size + 10;
                    return redraw();
                }
                if (image_size + text_rect.bottom + 10 > position_.height())
                {
                    position_.bottom = position_.top + text_rect.bottom + image_size + 10;
                    return redraw();
                }
                if (text_rect.bottom + 6 > position_.height())
                {
                    position_.bottom = position_.top + text_rect.bottom + 6;
                    return redraw();
                }

                image_left = control_pos.left + ((control_pos.width() - image_size) / 2);
                image_top = control_pos.top + ((control_pos.height() - text_rect.bottom - image_size - 5) / 2);
                text_left = control_pos.left + ((control_pos.width() - text_rect.right) / 2);
                text_top = image_top + image_size + 5;
            }
        break;
    }

    if (button_view_ != button_view::anchor && button_view_ != button_view::switcher && button_view_ != button_view::radio && button_view_ != button_view::sheet)
    {
        auto border_color = focused_
            ? theme_color(tcn, tv_focused_border, theme_)
            : (!active ? theme_color(tcn, tv_border, theme_) : theme_color(tcn, tv_hover_border, theme_));

        auto fill_color = enabled_ ? (active || turned_ ? theme_color(tcn, tv_active, theme_) : theme_color(tcn, tv_calm, theme_)) : theme_color(tcn, tv_disabled, theme_);

        gr.draw_rect(control_pos, border_color, fill_color, theme_dimension(tcn, tv_border_width, theme_), theme_dimension(tcn, tv_round, theme_));
    }
    
    if (button_view_ != button_view::text && button_view_ != button_view::anchor && image_)
    {
        image_->set_position( { image_left,
            image_top,
            image_left + (button_view_ != button_view::switcher && button_view_ != button_view::radio ? image_size : image_->width()),
            image_top + (button_view_ != button_view::switcher  && button_view_ != button_view::radio ? image_size : image_->height()) });
        image_->draw(gr, { 0 });
    }

    if (button_view_ != button_view::image)
    {
        auto color_ = theme_color(tcn, tv_text, theme_);

        if (button_view_ == button_view::anchor)
        {
            color_ = theme_color(tcn, tv_anchor, theme_);
            font_.decorations_ = decorations::underline;
        }

        if (!enabled_ && (button_view_ == button_view::anchor || button_view_ == button_view::sheet))
        {
            color_ = theme_color(tcn, tv_disabled, theme_);
        }

        gr.draw_text({ text_left, text_top }, caption, 
            color_,
            font_);
    }

    if (button_view_ == button_view::sheet)
    {
        gr.draw_rect({ control_pos.left, control_pos.bottom - 2, control_pos.right, control_pos.bottom }, turned_ ? theme_color(tcn, enabled_ ? tv_calm : tv_disabled, theme_) : theme_color(window::tc, window::tv_background, theme_));
    }
}

void button::receive_event(const event &ev)
{
    if (!showed_ || !enabled_)
    {
        return;
    }

    if (ev.type == event_type::mouse)
    {
        switch (ev.mouse_event_.type)
        {
            case mouse_event_type::enter:
            {
                active = true;
                auto parent__ = parent_.lock();
                if (parent__)
                {
                    set_cursor(parent__->context(), button_view_ != button_view::anchor ? cursor::default_ : cursor::hand);
                }
                redraw();

                if (button_view_ == button_view::image && !caption.empty())
                {
                    tooltip_->show_on_control(*this, 5);
                }
            }
            break;
            case mouse_event_type::leave:
            {
                pushed = false;

                if (button_view_ == button_view::image && !caption.empty())
                {
                    tooltip_->hide();
                }

                active = false;
                auto parent__ = parent_.lock();
                if (parent__)
                {
                    set_cursor(parent__->context(), cursor::default_);
                }
                redraw();
            }
            break;
            case mouse_event_type::left_down:
                pushed = true;
            break;
            case mouse_event_type::left_up:
                if (pushed)
                {
                    active = false;
                    tooltip_->hide();

                    if (button_view_ == button_view::switcher || button_view_ == button_view::radio)
                    {
                        turn(!turned_);
                    }

                    if (click_callback)
                    {
                        click_callback();
                    }

                    pushed = false;
                }
            break;
        }
    }
    else if (ev.type == event_type::internal)
    {
        switch (ev.internal_event_.type)
        {
            case internal_event_type::set_focus:
                if (focusing_)
                {
                    focused_ = true;

                    redraw();
                }
            break;
            case internal_event_type::remove_focus:
                focused_ = false;
                redraw();
            break;
            case internal_event_type::execute_focused:
                if (button_view_ == button_view::switcher || button_view_ == button_view::radio)
                {
                    turn(!turned_);
                }
                if (click_callback)
                {
                    click_callback();
                }
            break;
        }
    }
}

void button::set_position(rect position__)
{
    position_ = position__;
}

rect button::position() const
{
    return get_control_position(position_, parent_);
}

void button::set_parent(std::shared_ptr<window> window_)
{
    active = false;
    focused_ = false;

    parent_ = window_;
    window_->add_control(tooltip_, tooltip_->position());
    my_subscriber_id = window_->subscribe(std::bind(&button::receive_event, this, std::placeholders::_1),
        wui::flags_map<wui::event_type>(2, wui::event_type::internal, wui::event_type::mouse),
        shared_from_this());
}

std::weak_ptr<window> button::parent() const
{
    return parent_;
}

void button::clear_parent()
{
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(tooltip_);
        parent__->unsubscribe(my_subscriber_id);
    }
    parent_.reset();
}

void button::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool button::topmost() const
{
    return topmost_;
}

bool button::focused() const
{
    return enabled_ && showed_ && focused_;
}

bool button::focusing() const
{
    return enabled_ && showed_ && focusing_;
}

error button::get_error() const
{
    return err;
}

void button::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void button::update_theme(std::shared_ptr<i_theme> theme__)
{
    if (theme_ && !theme__)
    {
        return;
    }
    theme_ = theme__;

    tooltip_->update_theme(theme_);

    if (button_view_ == button_view::switcher)
    {
        image_->change_image(theme_image(turned_ ? ti_switcher_on : ti_switcher_off));
        update_err("button::update_theme[switcher]", image_->get_error());
    }
    if (button_view_ == button_view::radio)
    {
        image_->change_image(theme_image(turned_ ? ti_radio_on : ti_radio_off));
        update_err("button::update_theme[radio]", image_->get_error());
    }
    else if (image_)
    {
        image_->update_theme(theme_);
    }

    redraw();
}

void button::show()
{
    if (!showed_)
    {
        showed_ = true;
        redraw();
    }
}

void button::hide()
{
    showed_ = false;
    tooltip_->hide();
    auto parent__ = parent_.lock();
    if (parent__)
    {
        auto pos = position();
        pos.widen(theme_dimension(tcn, tv_border_width, theme_));
        parent__->redraw(pos, true);
    }
}

bool button::showed() const
{
    return showed_;
}

void button::enable()
{
    enabled_ = true;
    redraw();
}

void button::disable()
{
    enabled_ = false;
    redraw();
}

bool button::enabled() const
{
    return enabled_;
}

void button::set_caption(std::string_view caption_)
{
    caption = caption_;
    tooltip_->set_text(caption_);

    text_rect = { 0 };
    
    redraw();
}

void button::set_button_view(button_view button_view__)
{
    button_view_ = button_view__;

    redraw();
}

#ifdef _WIN32
void button::set_image(int32_t resource_index)
{
    if (image_)
    {
        image_->change_image(resource_index);
    }
    else
    {
        image_ = std::make_shared<image>(resource_index);
    }

    update_err("button::set_image[from resource]", image_->get_error());
    redraw();
}
#endif

void button::set_image(std::string_view file_name)
{
    if (image_)
    {
        image_->change_image(file_name);
    }
    else
    {
        image_ = std::make_shared<image>(file_name);
    }
    
    update_err("button::set_image[from file]", image_->get_error());
    redraw();
}

void button::set_image(const std::vector<uint8_t> &image_data)
{
    if (image_)
    {
        image_->change_image(image_data);
    }
    else
    {
        image_ = std::make_shared<image>(image_data);
    }

    update_err("button::set_image[from data]", image_->get_error());
    redraw();
}

void button::enable_focusing()
{
    focusing_ = true;
}

void button::disable_focusing()
{
    focusing_ = false;
}

void button::turn(bool on)
{
    turned_ = on;
    switch (button_view_)
    {
        case button_view::switcher:
            image_->change_image(theme_image(turned_ ? ti_switcher_on : ti_switcher_off));
            update_err("button::turn", image_->get_error());
        break;
        case button_view::radio:
            image_->change_image(theme_image(turned_ ? ti_radio_on : ti_radio_off));
            update_err("button::turn", image_->get_error());
        break;
        default:
        break;
    }
    redraw();
}

bool button::turned() const
{
    return turned_;
}

void button::set_callback(std::function<void(void)> click_callback_)
{
    click_callback = click_callback_;
}

void button::redraw()
{
    if (showed_)
    {
        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto pos = position();
            pos.widen(theme_dimension(tcn, tv_border_width, theme_));
            parent__->redraw(pos, true);
        }
    }
}

void button::update_err(std::string_view place, const error &input_err)
{
    err = input_err;
    err.component = std::string(place) + "::" + input_err.component;
}

}
