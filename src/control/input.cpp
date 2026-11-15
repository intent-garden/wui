//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include <wui/control/input.hpp>

#include <wui/window/window.hpp>

#include <wui/theme/theme.hpp>

#include <wui/system/tools.hpp>

#include <wui/system/clipboard_tools.hpp>

#include <wui/locale/locale.hpp>

#include <wui/common/flag_helpers.hpp>

#include <wui/common/dbgtrace.hpp>

#include <boost/nowide/convert.hpp>
#include <utf8/utf8.h>

#include <regex>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

#undef min
#undef max

namespace wui
{

static const int32_t INPUT_HORIZONTAL_INDENT = 5;
static const int32_t SCROLL_SIZE = 14;

input::input(std::string_view text__, input_view input_view__, input_content input_content__, int32_t symbols_limit_, std::string_view theme_control_name_, std::shared_ptr<i_theme> theme__)
    : input_view_(input_view__),
    input_content_(input_content__),
    symbols_limit(symbols_limit_),
    change_callback(),
    tcn(theme_control_name_),
    theme_(theme__),
    position_{ 0 },
    parent_(),
    my_control_sid(), my_plain_sid(),
    timer_(std::bind(&input::redraw_cursor, this)),
    menu_(std::make_shared<menu>(menu::tc, theme_)),
    vert_scroll(std::make_shared<scroll>(0, 0, orientation::vertical, std::bind(&input::on_vert_scroll, this, std::placeholders::_1, std::placeholders::_2), scroll::tc, theme__)),
    hor_scroll(std::make_shared<scroll>(0, 0, orientation::horizontal, std::bind(&input::on_hor_scroll, this, std::placeholders::_1, std::placeholders::_2), scroll::tc, theme__)),
    scroll_offset_x(0), scroll_offset_y(0),
    showed_(true), enabled_(true), topmost_(false),
    active(false), focused_(false),
    cursor_visible(false),
    selecting(false),
    mem_gr(),
    auto_scroll_timer_(std::make_shared<timer>([this]() { on_auto_scroll(); }))
{
    update_lines(text__);
    reset_state();

    menu_->set_items({
            { 0, menu_item_state::normal, locale(tc, cl_cut).data(), "Ctrl+X", nullptr, {}, [this](int32_t) { buffer_cut(); } },
            { 1, menu_item_state::normal, locale(tc, cl_copy).data(), "Ctrl+C", nullptr, {}, [this](int32_t) { buffer_copy(); } },
            { 2, menu_item_state::normal, locale(tc, cl_paste).data(), "Ctrl+V", nullptr, {}, [this](int32_t) { buffer_paste(); } }
        });
}

input::~input()
{
    stop_auto_scroll();
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(shared_from_this());
    }
}

int32_t get_text_width(std::string text, size_t text_length, const font &font_)
{
    if (text.empty() || text_length == 0) return 0;
    text.resize(text_length);
    auto text_rect = measure_text(text, font_);
    return text_rect.right;
}

// Auxiliary function: comparing cursor positions (row, col)
inline bool cursor_less(size_t row1, size_t col1, size_t row2, size_t col2)
{
    return row1 < row2 || (row1 == row2 && col1 < col2);
}

// Auxiliary functions for working with byte indexes (as in single-line mode)
static size_t get_byte_pos_for_char_pos(const std::string& s, size_t char_pos)
{
    if (s.empty())
    {
        return 0;
    }

    auto it = s.begin();
    size_t actual_chars = utf8::distance(s.begin(), s.end());
    if (char_pos >= actual_chars)
    {
        return s.size(); // Returning the end of the string
    }

    utf8::advance(it, char_pos, s.end());
    return std::distance(s.begin(), it);
}

font input::get_font()
{
    auto font_ = theme_font(tcn, tv_font, theme_);
    if (input_view_ == input_view::password)
    {
#ifdef _WIN32
        font_.name = "Courier New";
#elif __linux__
        font_.name = "monospace";
#endif
    }
    return font_;
}

bool input::update_mem_gr()
{
    if (!mem_gr) return false;

    auto width = position_.width(), height = position_.height();

    auto current = mem_gr->max_size();

    if (current.width() < width || current.height() < height)
    {
        mem_gr->release();
        mem_gr->init({ 0, 0, width, height }, theme_color(tcn, tv_background, theme_));
    }

    return true;
}

void input::draw(graphic &gr, rect)
{
    if (!showed_ || position_.is_null())
    {
        return;
    }

    auto control_pos = position();

    auto font_ = get_font();

    auto border_width = theme_dimension(tcn, tv_border_width, theme_);
    int line_height = font_.size;

    auto content_height = control_pos.height() - (hor_scroll->showed() ? SCROLL_SIZE : 0);

    bool ok = update_mem_gr();
    if (!ok) return;

    mem_gr->clear();

    if (line_height > 0)
    {
        // We start from the scroll position
        int y = - (scroll_offset_y % line_height);
        size_t start_line = scroll_offset_y / line_height;
        int visible_bottom = content_height;
        size_t count = input_view_ == input_view::multiline ? lines_.size() : 1;
        for (size_t i = start_line; i < count; ++i)
        {
            int actual_y = y + static_cast<int>(i - start_line) * line_height;
            if (actual_y >= visible_bottom) break;

            if (input_view_ != input_view::multiline)
            {
                actual_y = position_.height() > line_height ? (position_.height() - line_height) / 2 : border_width;
            }

            // Highlighting the selection
            bool has_sel = false, newline_selected = false;
            auto display_line = input_view_ == input_view::password
                ? std::string(utf8::distance(lines_[i].begin(), lines_[i].end()), '*') : lines_[i];
            size_t sel_start = 0, sel_end = 0;
            if (!(select_start_row == select_end_row && select_start_col == select_end_col))
            {
                size_t srow = select_start_row, scol = select_start_col, erow = select_end_row, ecol = select_end_col;
                if (cursor_less(erow, ecol, srow, scol)) { std::swap(srow, erow), std::swap(scol, ecol); }
                newline_selected = i >= srow && i < erow;
                if (i > srow && i < erow)
                {
                    has_sel = true; sel_start = 0; sel_end = utf8::distance(lines_[i].begin(), lines_[i].end());
                }
                else if (i == srow && i == erow && scol != ecol)
                {
                    has_sel = true; sel_start = scol; sel_end = ecol;
                    if (sel_start > sel_end) std::swap(sel_start, sel_end);
                }
                else if (i == srow && i < erow)
                {
                    has_sel = true; sel_start = scol; sel_end = utf8::distance(lines_[i].begin(), lines_[i].end());
                }
                else if (i == erow && i > srow)
                {
                    has_sel = true; sel_start = 0; sel_end = ecol;
                }
            }
            if (has_sel && (sel_start < sel_end || newline_selected) && static_cast<size_t>(sel_end) <= static_cast<size_t>(utf8::distance(lines_[i].begin(), lines_[i].end())))
            {
                size_t start_byte = get_byte_pos_for_char_pos(display_line, sel_start);
                size_t end_byte = get_byte_pos_for_char_pos(display_line, sel_end);
                int x1 = measure_text(display_line.substr(0, start_byte), font_, mem_gr.get()).right - scroll_offset_x + INPUT_HORIZONTAL_INDENT;
                int x2 = measure_text(display_line.substr(0, end_byte), font_, mem_gr.get()).right - scroll_offset_x + INPUT_HORIZONTAL_INDENT;
                if (newline_selected) x2 += std::max(1, measure_text(" ", font_, mem_gr.get()).right);
                mem_gr->draw_rect({ x1, actual_y, x2, actual_y + line_height }, theme_color(tcn, tv_selection, theme_));
            }
            if (input_view_ != input_view::password)
            {
                mem_gr->draw_text({ INPUT_HORIZONTAL_INDENT - scroll_offset_x, actual_y }, lines_[i], theme_color(tcn, tv_text, theme_), font_);
            }
            else
            {
                std::string str; str.resize(utf8::distance(lines_[i].begin(), lines_[i].end()), '*');
                mem_gr->draw_text({ INPUT_HORIZONTAL_INDENT - scroll_offset_x, actual_y }, str, theme_color(tcn, tv_text, theme_), font_);
            }

            // Cursor
            if (cursor_visible && focused_ && input_view_ != input_view::readonly && i == cursor_row)
            {
                size_t max_col = utf8::distance(lines_[i].begin(), lines_[i].end());
                size_t safe_cursor_col = std::min(cursor_col, max_col);
                size_t cursor_byte = get_byte_pos_for_char_pos(display_line, safe_cursor_col);
                int cursor_x = measure_text(display_line.substr(0, cursor_byte), font_, mem_gr.get()).right - scroll_offset_x + INPUT_HORIZONTAL_INDENT;
                mem_gr->draw_line({ cursor_x, actual_y, cursor_x, actual_y + line_height }, theme_color(tcn, tv_cursor, theme_));
            }
        }
        // Copying the offscreen buffer to the parent context
        gr.draw_graphic({ control_pos.left,
            control_pos.top,
            control_pos.width(),
            control_pos.height() }, *mem_gr, 0, 0);

        // Rendering scrollbars
        if (input_view_ == input_view::multiline)
        {
            if (vert_scroll->showed()) vert_scroll->draw(gr, {});
            if (hor_scroll->showed()) hor_scroll->draw(gr, {});
        }
    }

    /// Draw the frame
    auto border_color = focused_
        ? theme_color(tcn, tv_focused_border, theme_)
        : (!active ? theme_color(tcn, tv_border, theme_) : theme_color(tcn, tv_hover_border, theme_));
    gr.draw_rect(control_pos,
        border_color,
        make_color(0, 0, 0, 255),
        border_width,
        theme_dimension(tcn, tv_round, theme_));
}

// Auxiliary function for multiline
std::pair<size_t, size_t> input::calculate_mouse_cursor_position(int x, int y)
{
    auto font_ = get_font();
    auto pos = position();
    int rel_y = std::max(0, y - pos.top + scroll_offset_y);
    size_t row = input_view_ == input_view::multiline
        ? std::min(static_cast<size_t>(rel_y / std::max(1, font_.size)), lines_.size() - 1) : 0;
    int rel_x = x - pos.left - INPUT_HORIZONTAL_INDENT + scroll_offset_x;
    const auto& line = lines_[row];
    size_t count = utf8::distance(line.begin(), line.end());
    int previous_width = 0;
    for (size_t col = 0; col < count; ++col)
    {
        auto prefix = input_view_ == input_view::password ? std::string(col + 1, '*')
            : line.substr(0, get_byte_pos_for_char_pos(line, col + 1));
        int width = measure_text(prefix, font_).right;
        if (rel_x < (previous_width + width) / 2) return {row, col};
        previous_width = width;
    }
    return {row, count};
}

void input::drag_selection(int x, int y)
{
    auto pos = position();
    drag_x_ = x;
    auto [row, col] = calculate_mouse_cursor_position(x, y);
    cursor_row = select_end_row = row;
    cursor_col = select_end_col = col;
    preferred_col_valid_ = false;
    cursor_visible = true;
    if (input_view_ == input_view::multiline && y < pos.top) start_auto_scroll(true);
    else if (input_view_ == input_view::multiline && y >= pos.bottom) start_auto_scroll(false);
    else if (x < pos.left) start_auto_hscroll(true);
    else if (x >= pos.right) start_auto_hscroll(false);
    else stop_auto_scroll();
    scroll_to_cursor();
    redraw();
}

void input::receive_control_events(const event &ev)
{
    if (!showed_ || !enabled_)
    {
        return;
    }

    // Scrollbar event handling for multiline
    if (input_view_ == input_view::multiline && ev.type == event_type::mouse && !selecting) {
        if (ev.mouse_event_.type == mouse_event_type::move || ev.mouse_event_.type == mouse_event_type::enter) {
            auto parent__ = parent_.lock();
            if (parent__) {
                if ((vert_scroll->showed() && vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y)) || (hor_scroll->showed() && hor_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y))) {
                    set_cursor(parent__->context(), cursor::default_);
                } else {
                    set_cursor(parent__->context(), cursor::ibeam);
                }
            }
            // Enter/leave emulation for scrollbars
            bool vert_hover = (vert_scroll->showed() && vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y));
            bool hor_hover = (hor_scroll->showed() && hor_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y));
            if (vert_hover && !prev_vert_hover) {
                event enter_ev = ev;
                enter_ev.mouse_event_.type = mouse_event_type::enter;
                vert_scroll->receive_control_events(enter_ev);
            } else if (!vert_hover && prev_vert_hover) {
                event leave_ev = ev;
                leave_ev.mouse_event_.type = mouse_event_type::leave;
                vert_scroll->receive_control_events(leave_ev);
            }
            if (hor_hover && !prev_hor_hover) {
                event enter_ev = ev;
                enter_ev.mouse_event_.type = mouse_event_type::enter;
                hor_scroll->receive_control_events(enter_ev);
            } else if (!hor_hover && prev_hor_hover) {
                event leave_ev = ev;
                leave_ev.mouse_event_.type = mouse_event_type::leave;
                hor_scroll->receive_control_events(leave_ev);
            }
            prev_vert_hover = vert_hover;
            prev_hor_hover = hor_hover;
        }
        // Checking whether the cursor is above the scrollbars for mouse events
        if (vert_scroll->showed() && vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y)) {
            vert_scroll->receive_control_events(ev);
            return;
        }
        if (hor_scroll->showed() && hor_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y)) {
            hor_scroll->receive_control_events(ev);
            return;
        }
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
                    set_cursor(parent__->context(), cursor::ibeam);
                }
                stop_auto_scroll();
                redraw();
            }
            break;
            case mouse_event_type::leave:
            {
                active = false;
                redraw();
                if (selecting) drag_selection(ev.mouse_event_.x, ev.mouse_event_.y);

                auto parent__ = parent_.lock();
                if (parent__)
                {
                    set_cursor(parent__->context(), cursor::default_);
                }
            }
            break;
            case mouse_event_type::left_down:
            {
                auto [row, col] = calculate_mouse_cursor_position(ev.mouse_event_.x, ev.mouse_event_.y);
                cursor_row = row;
                cursor_col = col;
                selecting = true;
                preferred_col_valid_ = false;
                cursor_visible = true;
                stop_auto_scroll();
                select_start_row = cursor_row;
                select_start_col = cursor_col;
                select_end_row = cursor_row;
                select_end_col = cursor_col;
                redraw();
                scroll_to_cursor();
            }
            break;
            case mouse_event_type::left_up:
                selecting = false;
                stop_auto_scroll();
                menu_->hide();
            break;
            case mouse_event_type::right_up:
            {
                bool has_selection = !(select_start_row == select_end_row && select_start_col == select_end_col);
                menu_->update_item({ 0, has_selection && input_view_ != input_view::readonly && input_view_ != input_view::password ? menu_item_state::normal : menu_item_state::disabled,
                    locale(tc, cl_cut).data(), "Ctrl+X", nullptr, {}, [this](int32_t) { buffer_cut(); parent_.lock()->set_focused(shared_from_this()); } });
                menu_->update_item({ 1, has_selection && input_view_ != input_view::password ? menu_item_state::normal : menu_item_state::disabled,
                    locale(tc, cl_copy).data(), "Ctrl+C", nullptr, {}, [this](int32_t) { buffer_copy(); parent_.lock()->set_focused(shared_from_this()); } });
                menu_->update_item({ 2, input_view_ != input_view::readonly ? menu_item_state::normal : menu_item_state::disabled,
                    locale(tc, cl_paste).data(), "Ctrl+V", nullptr, {}, [this](int32_t) { buffer_paste(); parent_.lock()->set_focused(shared_from_this()); } });

                // The popup temporarily takes focus; its commands need this selection.
                opening_menu_ = true;
                menu_->show_on_control(shared_from_this(), 0, ev.mouse_event_.x, ev.mouse_event_.y);
                opening_menu_ = false;
            }
            break;
            case mouse_event_type::move:
                if (selecting) drag_selection(ev.mouse_event_.x, ev.mouse_event_.y);
            break;
            case mouse_event_type::left_double:
                select_current_word(ev.mouse_event_.x, ev.mouse_event_.y);
            break;
            case mouse_event_type::wheel:
                if (input_view_ != input_view::multiline || !vert_scroll->showed()) break;
                if (ev.mouse_event_.wheel_delta > 0) {
                    vert_scroll->scroll_up();
                } else {
                    vert_scroll->scroll_down();
                }
            break;
            default: break;
        }
    }
    else if (ev.type == event_type::keyboard)
    {
        const auto& key = ev.keyboard_event_;
        if (key.type == keyboard_event_type::down)
        {
            timer_.stop();
            cursor_visible = true;
            const bool shift = key.modifier == vk_lshift || key.modifier == vk_rshift;
            switch (key.key[0])
            {
                case vk_left: case vk_right: case vk_up: case vk_down:
                case vk_home: case vk_end:
                case vk_page_up: case vk_npage_up: case vk_page_down: case vk_npage_down:
                    move_cursor(key.key[0], shift);
                    break;
                case vk_back: case vk_del:
                    if (input_view_ != input_view::readonly)
                    {
                        if (!has_selection()) move_cursor(key.key[0] == vk_back ? vk_left : vk_right, true);
                        if (clear_selected_text()) finish_edit();
                    }
                    break;
                case vk_return: case vk_rreturn:
                    if (input_view_ == input_view::multiline) insert_text("\n");
                    break;
                default: break;
            }
        }
        else if (key.type == keyboard_event_type::up && input_view_ != input_view::readonly) timer_.start(500);
        else if (key.type == keyboard_event_type::key)
        {
            switch (static_cast<unsigned char>(key.key[0]))
            {
                case 0x03: return buffer_copy();
                case 0x18: return buffer_cut();
                case 0x16: return buffer_paste();
                case 0x01: return select_all();
                default: break;
            }
            if (static_cast<unsigned char>(key.key[0]) >= 0x20 && key.key[0] != 0x7f)
                insert_text(std::string_view(key.key, key.key_size));
        }
    }
    else if (ev.type == event_type::internal)
    {
        switch (ev.internal_event_.type)
        {
            case internal_event_type::set_focus:
                focused_ = true;

                redraw();

                if (input_view_ != input_view::readonly) timer_.start(500);
            break;
            case internal_event_type::remove_focus:
                focused_ = false;

                cursor_visible = false;

                selecting = false;
                if (!opening_menu_)
                    select_start_row = select_start_col = select_end_row = select_end_col = 0;
                stop_auto_scroll();

                timer_.stop();

                redraw();
            break;
        }
    }
}

void input::receive_plain_events(const event &ev)
{
    if (ev.type != event_type::mouse) return;
    if (ev.mouse_event_.type == mouse_event_type::left_up)
    {
        if (selecting) drag_selection(ev.mouse_event_.x, ev.mouse_event_.y);
        selecting = false;
        stop_auto_scroll();
    }
    else if (selecting && ev.mouse_event_.type == mouse_event_type::move &&
             !position().in(ev.mouse_event_.x, ev.mouse_event_.y))
        drag_selection(ev.mouse_event_.x, ev.mouse_event_.y);
}

void input::set_position(rect position__)
{
    position_ = position__;
    if (input_view_ == input_view::multiline) {
        auto border_width = theme_dimension(tcn, tv_border_width, theme_) / 2;
        vert_scroll->set_position({ position_.right - 14 - border_width,
            position_.top + border_width,
            position_.right - border_width,
            position_.bottom - border_width - 3 });
        hor_scroll->set_position({ position_.left + border_width,
            position_.bottom - 14 - border_width,
            position_.right - border_width,
            position_.bottom - border_width });
    }
    update_scroll_areas();
    scroll_to_cursor();
}

rect input::position() const
{
    return get_control_position(position_, parent_);
}

void input::set_parent(std::shared_ptr<window> window_)
{
    parent_ = window_;
    my_control_sid = window_->subscribe(std::bind(&input::receive_control_events, this, std::placeholders::_1),
        wui::flags_map<wui::event_type>(3, wui::event_type::internal, wui::event_type::mouse, wui::event_type::keyboard),
        shared_from_this());
    my_plain_sid = window_->subscribe(std::bind(&input::receive_plain_events, this, std::placeholders::_1), event_type::mouse);
    window_->add_control(menu_, { 0 });

    if (input_view_ == input_view::multiline)
    {
        window_->add_control(vert_scroll, { 0 });
        window_->add_control(hor_scroll, { 0 });
    }

    /// Create memory dc for inner content
    if (mem_gr) mem_gr.reset();
    system_context ctx = { 0 };
    auto parent__ = parent_.lock();
    if (parent__)
    {
        mem_gr = std::make_unique<graphic>(parent__->context());
    }
}

std::weak_ptr<window> input::parent() const
{
    return parent_;
}

void input::clear_parent()
{
    selecting = false;
    stop_auto_scroll();
    timer_.stop();
    auto parent__ = parent_.lock();
    if (parent__)
    {
        parent__->remove_control(menu_);
        if (input_view_ == input_view::multiline)
        {
            parent__->remove_control(vert_scroll);
            parent__->remove_control(hor_scroll);
        }
        parent__->unsubscribe(my_control_sid);
        parent__->unsubscribe(my_plain_sid);
    }
    parent_.reset();

    mem_gr.reset();
}

void input::set_topmost(bool yes)
{
    topmost_ = yes;
}

bool input::topmost() const
{
    return topmost_;
}

bool input::focused() const
{
    return focused_;
}

bool input::focusing() const
{
    return enabled_ && showed_;
}

error input::get_error() const
{
    return {};
}

void input::update_theme_control_name(std::string_view theme_control_name)
{
    tcn = theme_control_name;
    update_theme(theme_);
}

void input::update_theme(std::shared_ptr<i_theme> theme__)
{
    theme_ = theme__;
    if (mem_gr)
    {
        mem_gr->set_background_color(theme_color(tcn, tv_background, theme__));
    }
}

void input::show()
{
    showed_ = true;
    redraw();
}

void input::hide()
{
    showed_ = false;
    selecting = false;
    stop_auto_scroll();
    auto parent__ = parent_.lock();
    if (parent__)
    {
        auto pos = position();
        pos.widen(theme_dimension(tcn, tv_border_width, theme_));
        parent__->redraw(pos, true);
    }
}

bool input::showed() const
{
    return showed_;
}

void input::enable()
{
    enabled_ = true;
    redraw();
}

void input::disable()
{
    enabled_ = false;
    selecting = false;
    stop_auto_scroll();
    redraw();
}

bool input::enabled() const
{
    return enabled_;
}

void input::set_text(std::string_view text__)
{
    update_lines(text__);
    reset_state();
    redraw();
    if (change_callback) change_callback();
}

void input::set_input_view(input_view input_view__)
{
    auto value = text();
    auto parent = parent_.lock();
    if (parent && input_view_ != input_view__)
    {
        if (input_view__ == input_view::multiline)
        {
            parent->add_control(vert_scroll, {0});
            parent->add_control(hor_scroll, {0});
        }
        else if (input_view_ == input_view::multiline)
        {
            parent->remove_control(vert_scroll);
            parent->remove_control(hor_scroll);
        }
    }
    input_view_ = input_view__;
    cursor_visible = false;
    timer_.stop();
    if (focused_ && input_view_ != input_view::readonly) timer_.start(500);
    update_lines(value);
    set_position(position_);
    redraw();
}

input_view input::get_input_view() const
{
    return input_view_;
}

void input::set_input_content(input_content input_content__)
{
    input_content_ = input_content__;
}

void input::set_symbols_limit(int32_t symbols_limit_)
{
    symbols_limit = symbols_limit_;
}

void input::set_change_callback(std::function<void()> change_callback_)
{
    change_callback = change_callback_;
}

void input::set_return_callback(std::function<void()> return_callback_)
{
    return_callback = return_callback_;
}

const std::vector<std::string>& input::get_lines() const
{
    return lines_;
}

void input::redraw()
{
    if (showed_)
    {
        auto parent__ = parent_.lock();
        if (parent__)
        {
            auto pos = position();
            pos.widen(theme_dimension(tcn, tv_border_width, theme_));
            parent__->redraw(pos);
        }
    }
}

void input::redraw_cursor()
{
    if (!focused_ || input_view_ == input_view::readonly) return;
    cursor_visible = !cursor_visible;
    redraw();
}

static std::string normalize_input_text(std::string_view text, bool multiline)
{
    std::string result;
    result.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i)
    {
        char c = text[i];
        if (c == '\r')
        {
            if (i + 1 < text.size() && text[i + 1] == '\n') ++i;
            c = '\n';
        }
        result += c == '\n' && !multiline ? ' ' : c;
    }
    return result;
}

void input::update_lines(std::string_view text)
{
    auto normalized = normalize_input_text(text, input_view_ == input_view::multiline);
    lines_.clear();
    size_t begin = 0;
    for (size_t end; (end = normalized.find('\n', begin)) != std::string::npos; begin = end + 1)
        lines_.push_back(normalized.substr(begin, end - begin));
    lines_.push_back(normalized.substr(begin));
    reset_state();
    invalidate_max_width_cache();
    update_scroll_areas();
}

std::string input::text() const
{
    std::ostringstream oss;
    auto size = lines_.size();
    for (size_t i = 0; i < size; ++i) {
        oss << lines_[i];
        if (i + 1 < size) oss << '\n';
    }
    return oss.str();
}

void input::set_caret_position(size_t position)
{
    size_t characters = lines_.empty() ? 0 : lines_.size() - 1;
    for (const auto &line : lines_) characters += utf8::distance(line.begin(), line.end());
    if (position > characters) position = characters;

    cursor_row = 0;
    cursor_col = position;
    for (size_t i = 0; i < lines_.size(); ++i)
    {
        auto length = utf8::distance(lines_[i].begin(), lines_[i].end());
        if (cursor_col <= length) break;
        cursor_col -= length + 1;
        ++cursor_row;
    }

    select_start_row = select_end_row = cursor_row;
    select_start_col = select_end_col = cursor_col;
    preferred_col_valid_ = false;

    scroll_to_cursor();
    redraw();
}

size_t input::caret_position() const
{
    size_t position = cursor_col;
    for (size_t i = 0; i < cursor_row && i < lines_.size(); ++i)
        position += utf8::distance(lines_[i].begin(), lines_[i].end()) + 1;
    return position;
}

void input::reset_state()
{
    selecting = false;
    preferred_col_valid_ = false;
    stop_auto_scroll();
    scroll_offset_x = scroll_offset_y = 0;
    hor_scroll->set_scroll_pos(0);
    vert_scroll->set_scroll_pos(0);
    cursor_row = cursor_col = 0;
    select_start_row = select_start_col = select_end_row = select_end_col = 0;
}

// Deleting selected text in multiline
bool input::clear_selected_text()
{
    if (select_start_row == select_end_row && select_start_col == select_end_col)
        return false;
    size_t srow = select_start_row, scol = select_start_col, erow = select_end_row, ecol = select_end_col;
    if (cursor_less(erow, ecol, srow, scol)) std::swap(srow, erow), std::swap(scol, ecol);
    if (srow == erow) {
        // We get byte positions for correct deletion of UTF-8 characters
        size_t start_byte = get_byte_pos_for_char_pos(lines_[srow], scol);
        size_t end_byte = get_byte_pos_for_char_pos(lines_[srow], ecol);
        lines_[srow].erase(start_byte, end_byte - start_byte);
        cursor_row = srow;
        cursor_col = scol;
    } else {
        // The first line
        size_t start_byte = get_byte_pos_for_char_pos(lines_[srow], scol);
        lines_[srow].erase(start_byte);

        // The last line
        size_t end_byte = get_byte_pos_for_char_pos(lines_[erow], ecol);
        lines_[erow].erase(0, end_byte);

        // Combining the lines
        lines_[srow] += lines_[erow];
        lines_.erase(lines_.begin() + srow + 1, lines_.begin() + erow + 1);
        cursor_row = srow;
        cursor_col = scol;
        invalidate_max_width_cache();
    }
    select_start_row = select_start_col = select_end_row = select_end_col = 0;
    selecting = false;
    invalidate_max_width_cache();
    return true;
}

// Auxiliary functions for multiline
void input::select_all() {
    if (lines_.empty()) return;
    select_start_row = 0;
    select_start_col = 0;
    select_end_row = lines_.size() - 1;
    select_end_col = utf8::distance(lines_[select_end_row].begin(), lines_[select_end_row].end());
    cursor_row = select_end_row;
    cursor_col = select_end_col;
    preferred_col_valid_ = false;
    selecting = false;
    stop_auto_scroll();
    redraw();
    scroll_to_cursor();
}

void input::select_current_word(int x, int y)
{
    auto [row, col] = calculate_mouse_cursor_position(x, y);
    const auto& line = lines_[row];
    size_t count = utf8::distance(line.begin(), line.end());
    if (col)
    {
        auto prefix = input_view_ == input_view::password ? std::string(col, '*')
            : line.substr(0, get_byte_pos_for_char_pos(line, col));
        int boundary = position().left + INPUT_HORIZONTAL_INDENT - scroll_offset_x + measure_text(prefix, get_font()).right;
        if (col == count || x < boundary) --col;
    }
    auto category = [&](size_t column) {
        unsigned char c = line[get_byte_pos_for_char_pos(line, column)];
        if (std::isspace(c)) return 0;
        if (c >= 0x80 || std::isalnum(c) || c == '_') return 1;
        return 2;
    };
    select_start_row = select_end_row = cursor_row = row;
    select_start_col = select_end_col = col;
    if (count)
    {
        int kind = category(col);
        while (select_start_col && category(select_start_col - 1) == kind) --select_start_col;
        while (select_end_col < count && category(select_end_col) == kind) ++select_end_col;
    }
    cursor_col = select_end_col;
    selecting = false;
    preferred_col_valid_ = false;
    stop_auto_scroll();
    redraw();
    scroll_to_cursor();
}

// Clipboard functions for multiline
void input::buffer_copy() {
    if (!parent_.lock() || (select_start_row == select_end_row && select_start_col == select_end_col) || input_view_ == input_view::password) {
        return;
    }

    size_t srow = select_start_row, scol = select_start_col, erow = select_end_row, ecol = select_end_col;
    if (cursor_less(erow, ecol, srow, scol)) std::swap(srow, erow), std::swap(scol, ecol);

    std::ostringstream oss;
    if (srow == erow) {
        // We get byte positions for correct copying of UTF-8 characters
        size_t start_byte = get_byte_pos_for_char_pos(lines_[srow], scol);
        size_t end_byte = get_byte_pos_for_char_pos(lines_[srow], ecol);
        oss << lines_[srow].substr(start_byte, end_byte - start_byte);
    } else {
        // The first line
        size_t start_byte = get_byte_pos_for_char_pos(lines_[srow], scol);
        oss << lines_[srow].substr(start_byte);

        // Middle lines
        for (size_t i = srow + 1; i < erow; ++i) {
            oss << '\n' << lines_[i];
        }

        // The last line
        if (erow > srow) {
            size_t end_byte = get_byte_pos_for_char_pos(lines_[erow], ecol);
            oss << '\n' << lines_[erow].substr(0, end_byte);
        }
    }

    clipboard_put(oss.str(), parent_.lock()->context());
}

void input::buffer_cut()
{
    if (!has_selection() || input_view_ == input_view::readonly || input_view_ == input_view::password) return;
    buffer_copy();
    if (clear_selected_text()) finish_edit();
}

void input::buffer_paste()
{
    auto parent = parent_.lock();
    if (parent && is_text_in_clipboard(parent->context()))
        insert_text(clipboard_get_text(parent->context()));
}

bool input::has_selection() const
{
    return select_start_row != select_end_row || select_start_col != select_end_col;
}

void input::finish_edit()
{
    selecting = false;
    preferred_col_valid_ = false;
    stop_auto_scroll();
    cursor_visible = true;
    invalidate_max_width_cache();
    update_scroll_areas();
    scroll_to_cursor();
    redraw();
    if (change_callback) change_callback();
}

void input::insert_text(std::string_view value)
{
    if (input_view_ == input_view::readonly || value.empty() || !utf8::is_valid(value.begin(), value.end())) return;
    auto inserted = normalize_input_text(value, input_view_ == input_view::multiline);
    for (unsigned char c : inserted)
    {
        if (input_content_ == input_content::integer && (c < '0' || c > '9')) return;
        if (input_content_ == input_content::numeric && std::string_view("-,.0123456789").find(c) == std::string_view::npos) return;
        if (input_content_ == input_content::hexadecimal && !std::isxdigit(c)) return;
    }
    auto original = text();
    auto offset = [this](size_t row, size_t col) {
        size_t result = get_byte_pos_for_char_pos(lines_[row], col);
        for (size_t i = 0; i < row; ++i) result += lines_[i].size() + 1;
        return result;
    };
    size_t begin = offset(cursor_row, cursor_col), end = begin;
    if (has_selection())
    {
        begin = offset(select_start_row, select_start_col);
        end = offset(select_end_row, select_end_col);
        if (begin > end) std::swap(begin, end);
    }
    std::string result = original.substr(0, begin) + inserted + original.substr(end);
    if (symbols_limit >= 0 && utf8::distance(result.begin(), result.end()) > symbols_limit) return;
    // Validate the complete replacement before touching the selection or text.
    size_t row = 0, col = 0;
    auto prefix = result.substr(0, begin + inserted.size());
    for (auto it = prefix.begin(); it != prefix.end();)
    {
        if (utf8::next(it, prefix.end()) == '\n') { ++row; col = 0; }
        else ++col;
    }
    update_lines(result);
    cursor_row = row;
    cursor_col = col;
    if (original != result) finish_edit();
    else { scroll_to_cursor(); redraw(); }
}

void input::move_cursor(uint8_t key, bool shift)
{
    size_t old_row = cursor_row, old_col = cursor_col;
    bool selected = has_selection();
    bool vertical = key == vk_up || key == vk_down || key == vk_page_up || key == vk_npage_up || key == vk_page_down || key == vk_npage_down;
    auto length = [this](size_t row) { return static_cast<size_t>(utf8::distance(lines_[row].begin(), lines_[row].end())); };
    if (vertical && !preferred_col_valid_) { preferred_col_ = cursor_col; preferred_col_valid_ = true; }
    if (!vertical) preferred_col_valid_ = false;
    if (!shift && selected && (key == vk_left || key == vk_right))
    {
        bool start_first = cursor_less(select_start_row, select_start_col, select_end_row, select_end_col);
        bool use_start = (key == vk_left) == start_first;
        cursor_row = use_start ? select_start_row : select_end_row;
        cursor_col = use_start ? select_start_col : select_end_col;
    }
    else if (key == vk_left)
    {
        if (cursor_col) --cursor_col;
        else if (cursor_row) { --cursor_row; cursor_col = length(cursor_row); }
    }
    else if (key == vk_right)
    {
        if (cursor_col < length(cursor_row)) ++cursor_col;
        else if (cursor_row + 1 < lines_.size()) { ++cursor_row; cursor_col = 0; }
    }
    else if (key == vk_home) cursor_col = 0;
    else if (key == vk_end) cursor_col = length(cursor_row);
    else if (vertical && input_view_ == input_view::multiline)
    {
        size_t step = 1;
        if (key != vk_up && key != vk_down)
            step = std::max(1, (position().height() - 2 * theme_dimension(tcn, tv_border_width, theme_) - SCROLL_SIZE) / std::max(1, get_font().size));
        bool up = key == vk_up || key == vk_page_up || key == vk_npage_up;
        cursor_row = up ? (cursor_row > step ? cursor_row - step : 0) : std::min(cursor_row + step, lines_.size() - 1);
        cursor_col = std::min(preferred_col_, length(cursor_row));
    }
    if (shift)
    {
        if (!selected) { select_start_row = old_row; select_start_col = old_col; }
        select_end_row = cursor_row;
        select_end_col = cursor_col;
    }
    else select_start_row = select_start_col = select_end_row = select_end_col = 0;
    scroll_to_cursor();
    redraw();
}

// Methods for working with scrolling
void input::update_scroll_areas()
{
    auto control_pos = position();
    auto border_width = theme_dimension(tcn, tv_border_width, theme_);
    auto font_ = get_font();
    int line_height = font_.size;

    // Calculate the maximum line width (using cache)
    int max_width = get_max_line_width();

    // Calculate the size of the text area
    int content_width = control_pos.width() - border_width * 2 - (input_view_ == input_view::multiline ? SCROLL_SIZE : 0);
    int content_height = control_pos.height() - border_width * 2 - (input_view_ == input_view::multiline ? SCROLL_SIZE : 0);

    // Count the vertical scrollbar
    int total_height = static_cast<int>(lines_.size()) * line_height;
    int vert_area = input_view_ == input_view::multiline ? std::max(0, total_height - content_height) : 0;
    vert_scroll->set_area(vert_area);

    // Count the horizontal scrollbar
    int hor_area = std::max(0, max_width - content_width);
    hor_scroll->set_area(hor_area);

    update_scroll_visibility();
}

void input::on_vert_scroll(scroll_state ss, int32_t v) {
    if (ss == scroll_state::up_end || ss == scroll_state::down_end || ss == scroll_state::moving) {
        scroll_offset_y = v;
        redraw();
    }
}

void input::on_hor_scroll(scroll_state ss, int32_t v) {
    if (ss == scroll_state::up_end || ss == scroll_state::down_end || ss == scroll_state::moving) {
        scroll_offset_x = v;
        redraw();
    }
}

void input::update_scroll_visibility() {
    if (input_view_ != input_view::multiline) {
        vert_scroll->hide();
        hor_scroll->hide();
        return;
    }

    auto control_pos = position();
    auto border_width = theme_dimension(tcn, tv_border_width, theme_);
    auto font_ = get_font();

    int line_height = font_.size;
    int total_height = static_cast<int>(lines_.size()) * line_height;
    int content_height = control_pos.height() - border_width * 2;

    // Calculating the maximum row width (using cache)
    int max_width = get_max_line_width();
    int content_width = control_pos.width() - border_width * 2;

    // Showing/hiding the vertical scroll
    bool need_vert_scroll = total_height > content_height && lines_.size() > 1;
    if (need_vert_scroll != vert_scroll->showed()) {
        if (need_vert_scroll) {
            vert_scroll->show();
        } else {
            vert_scroll->hide();
        }
    }

    // Showing/hiding the horizontal scroll
    bool need_hor_scroll = max_width > content_width;
    if (need_hor_scroll != hor_scroll->showed()) {
        if (need_hor_scroll) {
            hor_scroll->show();
        } else {
            hor_scroll->hide();
        }
    }
}

// Cache management for performance
int input::get_max_line_width()
{
    if (!max_width_dirty_ && cached_max_width_ >= 0) {
        return cached_max_width_;
    }

    auto font_ = get_font();

    int max_width = 0;

    for (const auto& line : lines_) {
        auto display = input_view_ == input_view::password ? std::string(utf8::distance(line.begin(), line.end()), '*') : line;
        auto text_width = get_text_width(display, display.size(), font_);
        max_width = std::max(max_width, text_width);
    }

    cached_max_width_ = max_width + INPUT_HORIZONTAL_INDENT * 2;
    max_width_dirty_ = false;

    return cached_max_width_;
}

void input::invalidate_max_width_cache()
{
    max_width_dirty_ = true;
    cached_max_width_ = -1;
}

// Auto-scroll functions for mouse selection
void input::start_auto_scroll(bool up)
{
    auto type = up ? auto_scroll_type::up : auto_scroll_type::down;
    if (auto_scroll_type_ == type) return;
    auto_scroll_type_ = type;
    auto_scroll_timer_->start(80);
}

void input::start_auto_hscroll(bool left)
{
    auto type = left ? auto_scroll_type::left : auto_scroll_type::right;
    if (auto_scroll_type_ == type) return;
    auto_scroll_type_ = type;
    auto_scroll_timer_->start(80);
}

void input::stop_auto_scroll()
{
    if (auto_scroll_timer_ && auto_scroll_type_ != auto_scroll_type::idle)
    {
        auto_scroll_type_ = auto_scroll_type::idle;
        auto_scroll_timer_->stop();
    }
}

void input::on_auto_scroll()
{
    if (!selecting || !showed_ || !enabled_) { stop_auto_scroll(); return; }
    size_t row = cursor_row, col = cursor_col;
    auto length = [this]() { return static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())); };
    auto mouse_column = [this]() {
        int height = std::max(1, get_font().size);
        int y = position().top + static_cast<int>(cursor_row) * height - scroll_offset_y + height / 2;
        return calculate_mouse_cursor_position(drag_x_, y).second;
    };
    switch (auto_scroll_type_)
    {
        case auto_scroll_type::up:
            if (cursor_row) { --cursor_row; cursor_col = mouse_column(); }
            else cursor_col = 0;
            cursor_col = std::min(cursor_col, length());
            break;
        case auto_scroll_type::down:
            if (cursor_row + 1 < lines_.size()) { ++cursor_row; cursor_col = mouse_column(); }
            else cursor_col = length();
            cursor_col = std::min(cursor_col, length());
            break;
        case auto_scroll_type::left:
            if (cursor_col) --cursor_col;
            break;
        case auto_scroll_type::right:
            if (cursor_col < length()) ++cursor_col;
            break;
        case auto_scroll_type::idle: return;
    }
    select_end_row = cursor_row;
    select_end_col = cursor_col;
    if (row == cursor_row && col == cursor_col) stop_auto_scroll();
    scroll_to_cursor();
    redraw();
}

void input::scroll_to_cursor()
{
    if (lines_.empty() || cursor_row >= lines_.size())
    {
        return;
    }

    if (input_view_ != input_view::multiline)
    {
        cursor_row = 0;
    }

    auto line = input_view_ == input_view::password
        ? std::string(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end()), '*') : lines_[cursor_row];

    auto control_pos = position();
    auto border_width = theme_dimension(tcn, tv_border_width, theme_);
    auto font_ = get_font();
    int line_height = font_.size;

    // We take into account the place for scrollbars
    bool show_vert_scroll = vert_scroll->showed();
    bool show_hor_scroll = hor_scroll->showed();

    int content_height = control_pos.height() - border_width * 2 - (show_hor_scroll ? SCROLL_SIZE : 0);
    int content_width = control_pos.width() - border_width * 2 - (show_vert_scroll ? SCROLL_SIZE : 0);

    int visible_left = scroll_offset_x;
    int visible_right = visible_left + content_width - 1;
    const int cursor_extra = 8;

    // Calculating the cursor position in pixels
    int cursor_y = static_cast<int>(cursor_row) * line_height;
    int cursor_x = 0;
    // Calculating the horizontal cursor position
    size_t max_col = line.empty() ? 0 : utf8::distance(line.begin(), line.end());
    size_t safe_cursor_col = std::min(cursor_col, max_col);
    size_t cursor_byte = line.empty() ? 0 : get_byte_pos_for_char_pos(line, safe_cursor_col);

    cursor_x = measure_text(line.substr(0, cursor_byte), font_).right;
    int line_width = measure_text(line, font_).right;

    if (safe_cursor_col == max_col)
    {
        int new_scroll = std::max(0, line_width + cursor_extra - content_width);
        hor_scroll->set_scroll_pos(new_scroll);
    }
    else
    {
        if (cursor_x < visible_left)
        {
            hor_scroll->set_scroll_pos(cursor_x);
        }
        else if (cursor_x + cursor_extra > visible_right)
        {
            hor_scroll->set_scroll_pos(cursor_x - content_width + cursor_extra);
        }
    }

    // Checking if you need to scroll vertically.
    if (input_view_ == input_view::multiline)
    {
        int visible_top = scroll_offset_y;
        int visible_bottom = visible_top + content_height;
        int total_height = static_cast<int>(lines_.size()) * line_height;
        int max_scroll = std::max(0, total_height - content_height);

        if (cursor_row == lines_.size() - 1)
        {
            vert_scroll->set_scroll_pos(max_scroll);
        }
        else if (cursor_y < visible_top)
        {
            vert_scroll->set_scroll_pos(cursor_y);
        }
        else if (cursor_y + line_height > visible_bottom)
        {
            int new_scroll = cursor_y + line_height - content_height;
            if (new_scroll < 0) new_scroll = 0;
            if (new_scroll > max_scroll) new_scroll = max_scroll;
            vert_scroll->set_scroll_pos(new_scroll);
        }
    }
}

void input::scroll_to_end()
{
    if (lines_.empty() || input_view_ != input_view::multiline)
    {
        return;
    }

    auto font_ = get_font();
    int32_t line_height = font_.size;

    vert_scroll->set_scroll_pos(line_height * static_cast<int32_t>(lines_.size()));
}

}
