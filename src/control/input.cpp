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

    auto content_height = control_pos.height() - (input_view_ == input_view::multiline ? SCROLL_SIZE : 0);

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

            if (count == 1)
            {
                actual_y = position_.height() > line_height ? (position_.height() - line_height) / 2 : border_width;
            }

            // Highlighting the selection
            bool has_sel = false;
            size_t sel_start = 0, sel_end = 0;
            if (!(select_start_row == select_end_row && select_start_col == select_end_col))
            {
                size_t srow = select_start_row, scol = select_start_col, erow = select_end_row, ecol = select_end_col;
                if (cursor_less(erow, ecol, srow, scol)) { std::swap(srow, erow), std::swap(scol, ecol); }
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
            if (has_sel && sel_start < sel_end && static_cast<size_t>(sel_end) <= static_cast<size_t>(utf8::distance(lines_[i].begin(), lines_[i].end())))
            {
                size_t start_byte = get_byte_pos_for_char_pos(lines_[i], sel_start);
                size_t end_byte = get_byte_pos_for_char_pos(lines_[i], sel_end);
                int x1 = measure_text(lines_[i].substr(0, start_byte), font_, mem_gr.get()).right - scroll_offset_x + INPUT_HORIZONTAL_INDENT;
                int x2 = measure_text(lines_[i].substr(0, end_byte), font_, mem_gr.get()).right - scroll_offset_x + INPUT_HORIZONTAL_INDENT;
                mem_gr->draw_rect({ x1, actual_y, x2, actual_y + line_height }, theme_color(tcn, tv_selection, theme_));
            }
            if (input_view_ != input_view::password)
            {
                mem_gr->draw_text({ INPUT_HORIZONTAL_INDENT - scroll_offset_x, actual_y }, lines_[i], theme_color(tcn, tv_text, theme_), font_);
            }
            else
            {
                std::string str; str.resize(lines_[i].size(), '*');
                mem_gr->draw_text({ INPUT_HORIZONTAL_INDENT - scroll_offset_x, actual_y }, str, theme_color(tcn, tv_text, theme_), font_);
            }
            
            // Cursor
            if (cursor_visible && i == cursor_row)
            {
                size_t max_col = utf8::distance(lines_[i].begin(), lines_[i].end());
                size_t safe_cursor_col = std::min(cursor_col, max_col);
                size_t cursor_byte = get_byte_pos_for_char_pos(lines_[i], safe_cursor_col);
                int cursor_x = measure_text(lines_[i].substr(0, cursor_byte), font_, mem_gr.get()).right - scroll_offset_x + INPUT_HORIZONTAL_INDENT;
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

bool is_number(std::string_view s)
{
    return s.find_first_not_of("-,.0123456789") != std::string::npos;
}

// Auxiliary function for multiline
std::pair<size_t, size_t> input::calculate_mouse_cursor_position(int x, int y)
{
    auto font_ = get_font();
    int line_height = font_.size;

    // Protection against division by zero
    if (line_height <= 0) {
        line_height = 1; // Fallback to minimum height
    }

    auto control_pos = position();
    auto border_width = theme_dimension(tcn, tv_border_width, theme_);
        
    // We take into account scrolling and borders
    int rel_y = y - control_pos.top + border_width + scroll_offset_y;
    
    // Protection against empty lines_
    if (lines_.empty()) {
        return {0, 0};
    }
    
    size_t row = std::min((size_t)(rel_y / line_height), lines_.size() - 1);
    
    int rel_x = x - control_pos.left + border_width - INPUT_HORIZONTAL_INDENT + scroll_offset_x;
    
    // We use character positions to measure text
    size_t char_count = utf8::distance(lines_[row].begin(), lines_[row].end());
    size_t col = 0;
    
    for (; col <= char_count; ++col)
    {
        size_t byte_pos = get_byte_pos_for_char_pos(lines_[row], col);
        int w = measure_text(lines_[row].substr(0, byte_pos), font_).right;
        if (w > rel_x) break;
    }
    
    if (col > 0) {
        --col;
    }
    if (col >= char_count) {
        col = char_count;
    }

    return {row, col};
}

void input::receive_control_events(const event &ev)
{
    if (!showed_ || !enabled_)
    {
        return;
    }

    // Scrollbar event handling for multiline
    if (input_view_ == input_view::multiline) {
        if (ev.type == event_type::mouse && (ev.mouse_event_.type == mouse_event_type::move || ev.mouse_event_.type == mouse_event_type::enter)) {
            auto parent__ = parent_.lock();
            if (parent__) {
                if (vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y) || hor_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y)) {
                    set_cursor(parent__->context(), cursor::default_);
                } else {
                    set_cursor(parent__->context(), cursor::ibeam);
                }
            }
            // Enter/leave emulation for scrollbars
            bool vert_hover = vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y);
            bool hor_hover = hor_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y);
            static bool prev_vert_hover = false;
            static bool prev_hor_hover = false;
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
        if (vert_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y)) {
            vert_scroll->receive_control_events(ev);
            return;
        }
        if (hor_scroll->position().in(ev.mouse_event_.x, ev.mouse_event_.y)) {
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
                if (selecting)
                {
                    select_end_col = select_start_col < select_end_col ? text().size() : 0;

                    auto control_pos = position();

                    if (ev.mouse_event_.x < control_pos.left && cursor_col > 0) {
                        start_auto_hscroll(true); // left
                        return;
                    }

                    if (ev.mouse_event_.x > control_pos.right && cursor_row < lines_[cursor_row].size()) {
                        start_auto_hscroll(false); // right
                        return;
                    }

                    if (input_view_ == input_view::multiline)
                    {
                        if (ev.mouse_event_.y < control_pos.top && cursor_row > 0) {
                            start_auto_scroll(true); // up
                            return;
                        }

                        if (ev.mouse_event_.y > control_pos.bottom && cursor_row + 1 < lines_.size()) {
                            start_auto_scroll(false); // down
                            return;
                        }
                    }
                }

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

                menu_->show_on_control(shared_from_this(), 0, ev.mouse_event_.x, ev.mouse_event_.y);
            }
            break;
            case mouse_event_type::move:
                if (selecting) {
                    // Обычная обработка для видимой области
                    auto [row, col] = calculate_mouse_cursor_position(ev.mouse_event_.x, ev.mouse_event_.y);
                    cursor_row = row;
                    cursor_col = col;
                    select_end_row = row;
                    select_end_col = col;
                    redraw();
                    scroll_to_cursor();
                }
            break;
            case mouse_event_type::left_double:
                select_current_word(ev.mouse_event_.x, ev.mouse_event_.y);
            break;
            case mouse_event_type::wheel:
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
        switch (ev.keyboard_event_.type)
        {
            case keyboard_event_type::down:
            {
                timer_.stop();
                cursor_visible = true;
                bool shift = (ev.keyboard_event_.modifier == vk_lshift || ev.keyboard_event_.modifier == vk_rshift);

                switch (ev.keyboard_event_.key[0]) {
                        case vk_left:
                            if (shift) {
                                size_t old_row = cursor_row, old_col = cursor_col;
                                if (cursor_col > 0) {
                                    --cursor_col;
                                } else if (cursor_row > 0) {
                                    --cursor_row;
                                    cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                                }
                                select_end_row = cursor_row; select_end_col = cursor_col;
                                if (!selecting) { select_start_row = old_row; select_start_col = old_col; selecting = true; }
                            } else {
                                if (cursor_col > 0) {
                                    --cursor_col;
                                } else if (cursor_row > 0) {
                                    --cursor_row;
                                    cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                                }
                                selecting = false; select_start_row = select_start_col = select_end_row = select_end_col = 0;
                            }
                            redraw();
                            scroll_to_cursor();
                            break;
                        case vk_right:
                            if (shift) {
                                size_t old_row = cursor_row, old_col = cursor_col;
                                size_t max_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                                if (cursor_col < max_col) {
                                    ++cursor_col;
                                } else if (cursor_row + 1 < lines_.size()) {
                                    ++cursor_row;
                                    cursor_col = 0;
                                }
                                select_end_row = cursor_row; select_end_col = cursor_col;
                                if (!selecting) { select_start_row = old_row; select_start_col = old_col; selecting = true; }
                            } else {
                                size_t max_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                                if (cursor_col < max_col) {
                                    ++cursor_col;
                                } else if (cursor_row + 1 < lines_.size()) {
                                    ++cursor_row;
                                    cursor_col = 0;
                                }
                                selecting = false; select_start_row = select_start_col = select_end_row = select_end_col = 0;
                            }
                            redraw();
                            scroll_to_cursor();
                            break;
                        case vk_up:
                            if (shift) {
                                size_t old_row = cursor_row, old_col = cursor_col;
                                if (cursor_row > 0) { --cursor_row; cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end()))); }
                                select_end_row = cursor_row; select_end_col = cursor_col;
                                if (!selecting) { select_start_row = old_row; select_start_col = old_col; selecting = true; }
                            } else {
                                if (cursor_row > 0) { --cursor_row; cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end()))); }
                                selecting = false; select_start_row = select_start_col = select_end_row = select_end_col = 0;
                            }
                            redraw();
                            scroll_to_cursor();
                            break;
                        case vk_down:
                            if (shift) {
                                size_t old_row = cursor_row, old_col = cursor_col;
                                if (cursor_row + 1 < lines_.size()) { 
                                    ++cursor_row; 
                                    cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end()))); 
                                } else {
                                    // Если уже на последней строке, двигаем курсор в конец строки
                                    cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                                }
                                select_end_row = cursor_row; select_end_col = cursor_col;
                                if (!selecting) { select_start_row = old_row; select_start_col = old_col; selecting = true; }
                            } else {
                                if (cursor_row + 1 < lines_.size()) { ++cursor_row; cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end()))); }
                                selecting = false; select_start_row = select_start_col = select_end_row = select_end_col = 0;
                            }
                            redraw();
                            scroll_to_cursor();
                            break;
                        case vk_home:
                            if (shift) {
                                size_t old_row = cursor_row, old_col = cursor_col;
                                cursor_col = 0;
                                select_end_row = cursor_row; select_end_col = cursor_col;
                                if (!selecting) { select_start_row = old_row; select_start_col = old_col; selecting = true; }
                            } else {
                                cursor_col = 0;
                                selecting = false; select_start_row = select_start_col = select_end_row = select_end_col = 0;
                            }
                            redraw();
                            scroll_to_cursor();
                            break;
                        case vk_end:
                            if (shift) {
                                size_t old_row = cursor_row, old_col = cursor_col;
                                // We use a symbolic position for the selection to work correctly
                                cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                                select_end_row = cursor_row; select_end_col = cursor_col;
                                if (!selecting) { select_start_row = old_row; select_start_col = old_col; selecting = true; }
                            } else {
                                cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                                selecting = false; select_start_row = select_start_col = select_end_row = select_end_col = 0;
                            }
                            redraw();
                            scroll_to_cursor();
                            break;
                        case vk_back:
                            if (clear_selected_text()) {
                                update_scroll_areas();
                                scroll_to_cursor();
                                redraw();
                                if (change_callback) change_callback();
                                break;
                            }
                            if (cursor_col > 0) {
                                auto prev_position = cursor_col;
                                --cursor_col;
                                // We get byte positions for correct deletion of UTF-8 characters
                                size_t prev_byte = get_byte_pos_for_char_pos(lines_[cursor_row], prev_position);
                                size_t curr_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
                                lines_[cursor_row].erase(curr_byte, prev_byte - curr_byte);
                            } else if (cursor_row > 0) {
                                --cursor_row;
                                cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                                lines_[cursor_row] += lines_[cursor_row + 1];
                                lines_.erase(lines_.begin() + cursor_row + 1);
                            }
                            invalidate_max_width_cache();
                            update_scroll_areas();
                            scroll_to_cursor();
                            redraw();
                            if (change_callback) change_callback();
                            break;
                        case vk_del:
                        {
                            if (clear_selected_text()) {
                                update_scroll_areas();
                                scroll_to_cursor();
                                redraw();
                                if (change_callback) change_callback();
                                break;
                            }
                            if (cursor_col < lines_[cursor_row].size()) {
                                // We get byte positions for correct deletion of UTF-8 characters
                                size_t start_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
                                size_t end_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col + 1);
                                lines_[cursor_row].erase(start_byte, end_byte - start_byte);
                            }
                            else if (cursor_row + 1 < lines_.size())
                            {
                                lines_[cursor_row] += lines_[cursor_row + 1];
                                lines_.erase(lines_.begin() + cursor_row + 1);
                            }
                            invalidate_max_width_cache();
                            update_scroll_areas();
                            scroll_to_cursor();
                            redraw();
                            if (change_callback) change_callback();
                        }
                            break;
                        case vk_return: case vk_rreturn:
                            if (input_view_ == input_view::multiline) {
                                // We get byte positions for correct operation with UTF-8
                                size_t cursor_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
                                std::string new_line = lines_[cursor_row].substr(cursor_byte);
                                lines_[cursor_row].erase(cursor_byte);
                                lines_.insert(lines_.begin() + cursor_row + 1, new_line);
                                ++cursor_row;
                                cursor_col = 0;
                                invalidate_max_width_cache();
                                update_scroll_areas();
                                scroll_to_cursor();
                                redraw();
                                if (change_callback) change_callback();
                            }
                            break;
                        case vk_page_up: case vk_npage_up:
                            if (cursor_row > 0) {
                                auto border_width = theme_dimension(tcn, tv_border_width, theme_);
                                auto font_ = get_font();
                                int line_height = font_.size;
                                
                                int content_height = position().height() - border_width * 2 - SCROLL_SIZE;

                                int visible_lines = std::max(1, content_height / line_height);
                                size_t new_row = cursor_row > (size_t)visible_lines ? cursor_row - visible_lines : 0;
                                cursor_row = new_row;
                                cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                                selecting = false; select_start_row = select_start_col = select_end_row = select_end_col = 0;
                                redraw();
                                scroll_to_cursor();
                            }
                            break;
                        case vk_page_down: case vk_npage_down:
                            if (cursor_row + 1 < lines_.size()) {
                                auto border_width = theme_dimension(tcn, tv_border_width, theme_);
                                auto font_ = get_font();
                                int line_height = font_.size;
                                int content_height = position().height() - border_width * 2 - SCROLL_SIZE;
                                int visible_lines = std::max(1, content_height / line_height);
                                size_t new_row = std::min(cursor_row + visible_lines, lines_.size() - 1);
                                cursor_row = new_row;
                                cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                                selecting = false; select_start_row = select_start_col = select_end_row = select_end_col = 0;
                                redraw();
                                scroll_to_cursor();
                            }
                            break;
                        default:
                            break;
                    }
                 }
            break;
            case keyboard_event_type::up:
                timer_.start(500);

                if (ev.keyboard_event_.key[0] == vk_lshift || ev.keyboard_event_.key[0] == vk_rshift)
                {
                    selecting = false;
                }
            break;
            case keyboard_event_type::key:
                if (ev.keyboard_event_.key[0] == 0x3)       // ctrl + c
                {
                    return buffer_copy();
                }
                else if (ev.keyboard_event_.key[0] == 0x18) // ctrl + x
                {
                    return buffer_cut();
                }
                else if (ev.keyboard_event_.key[0] == 0x16) // ctrl + v
                {
                    return buffer_paste();
                }
                else if (ev.keyboard_event_.key[0] == 0x1)  // ctrl + a
                {
                    return select_all();
                }
                else if (ev.keyboard_event_.key[0] == 0x7f) // ctrl + backspace
                {
                    if (input_view_ != input_view::readonly)
                    {
                        return update_lines("");
                    }
                }

                if (input_view_ == input_view::readonly ||
                    ev.keyboard_event_.key[0] == vk_tab ||
                    (symbols_limit != -1 && static_cast<int32_t>(text().size()) >= symbols_limit))
                {
                    return;
                }

                // Разрешить служебные клавиши (backspace, delete) независимо от input_content
                if (ev.keyboard_event_.key[0] == vk_back || ev.keyboard_event_.key[0] == vk_del)
                {
                    // Эти клавиши обрабатываются в keyboard_event_type::down, пропускаем здесь
                    return;
                }

                if (input_content_ == input_content::integer &&
                    !std::isdigit(ev.keyboard_event_.key[0]))
                {
                    return;
                }

                if (input_content_ == input_content::numeric &&
                    !is_number(ev.keyboard_event_.key))
                {
                    return;
                }

                if (clear_selected_text())
                {
                    redraw();
                    if (change_callback) change_callback();
                }
                
                if (symbols_limit != -1 && text().size() < (size_t)symbols_limit)
                {
                    if (ev.keyboard_event_.key[0] == 13 || ev.keyboard_event_.key[0] == 10)
                    {
                        return;
                    }
                    size_t insert_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
                    lines_[cursor_row].insert(insert_byte, ev.keyboard_event_.key, ev.keyboard_event_.key_size);
                    cursor_col += utf8::distance(ev.keyboard_event_.key, ev.keyboard_event_.key + ev.keyboard_event_.key_size);
                    invalidate_max_width_cache();
                    update_scroll_areas();
                    scroll_to_cursor();
                    redraw();
                    if (change_callback) change_callback();
                }    
            break;
        }
    }
    else if (ev.type == event_type::internal)
    {
        switch (ev.internal_event_.type)
        {
            case internal_event_type::set_focus:
                focused_ = true;

                redraw();

                timer_.start(500);
            break;
            case internal_event_type::remove_focus:
                focused_ = false;

                cursor_visible = false;

                selecting = false;
                select_start_col = select_end_col = 0;

                timer_.stop();

                redraw();
            break;
        }
    }
}

void input::receive_plain_events(const event &ev)
{
    if (ev.type == event_type::mouse && ev.mouse_event_.type == mouse_event_type::left_up)
    {
        selecting = false;
    }
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
        update_scroll_areas();
        scroll_to_cursor();
    }
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
    input_view_ = input_view__;
    reset_state();
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
    cursor_visible = !cursor_visible;
    redraw();
}

void input::update_lines(std::string_view text) {
    lines_.clear();
    if (text.empty()) {
        lines_.push_back("");
        cursor_row = 0;
        cursor_col = 0;
        invalidate_max_width_cache();
        return;
    }
    std::istringstream iss;
    iss.str(text.data());
    std::string line;
    while (std::getline(iss, line)) {
        lines_.push_back(line);
    }
    if (lines_.empty()) {
        lines_.push_back("");
    }
    cursor_row = 0;
    cursor_col = 0;
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

void input::reset_state()
{
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
    return true;
}

// Auxiliary functions for multiline
void input::select_all() {
    if (lines_.empty()) return;
    select_start_row = 0;
    select_start_col = 0;
    select_end_row = lines_.size() - 1;
    select_end_col = utf8::distance(lines_[select_end_row].begin(), lines_[select_end_row].end());
    redraw();
    scroll_to_cursor();
}

void input::select_current_word(int x, int y) {
    auto [row, col] = calculate_mouse_cursor_position(x, y);
    cursor_row = row;
    cursor_col = col;
    
    auto& line = lines_[row];
    select_start_row = select_end_row = row;
    select_start_col = select_end_col = col;
    
    // We are looking for the beginning of a word (using character positions)
    while (select_start_col > 0) {
        auto prev_char_pos = select_start_col - 1;
        auto prev_byte_pos = get_byte_pos_for_char_pos(line, prev_char_pos);
        if (prev_byte_pos < line.size() && line[prev_byte_pos] == ' ') {
            break;
        }
        select_start_col = prev_char_pos;
    }
    
    // Looking for the end of a word (using character positions)
    while (select_end_col < lines_[row].size()) {
        auto next_byte_pos = get_byte_pos_for_char_pos(line, select_end_col);
        if (next_byte_pos < line.size() && line[next_byte_pos] == ' ') {
            break;
        }
        select_end_col++;
    }
    
    selecting = true;
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

void input::buffer_cut() {
    if ((select_start_row == select_end_row && select_start_col == select_end_col) || input_view_ == input_view::readonly) {
        return;
    }

    buffer_copy();
    clear_selected_text();
    redraw();

    if (change_callback) change_callback();
}

void input::buffer_paste() {
    if (!parent_.lock() || input_view_ == input_view::readonly || !is_text_in_clipboard(parent_.lock()->context())) {
        return;
    }

    // We check that cursor_row does not exceed the boundaries
    if (lines_.empty()) {
        lines_.push_back("");
        cursor_row = 0;
        cursor_col = 0;
    } else if (cursor_row >= lines_.size()) {
        cursor_row = lines_.size() - 1;
        cursor_col = lines_[cursor_row].size();
    }

    clear_selected_text();

    auto paste_string = clipboard_get_text(parent_.lock()->context());
    
    // Splitting the inserted text into lines
    std::istringstream iss(paste_string);
    std::vector<std::string> paste_lines;
    std::string line;
    while (std::getline(iss, line)) {
        paste_lines.push_back(line);
    }
    if (paste_lines.empty()) paste_lines.push_back("");

    // Checking the character limit
    size_t total_chars = 0;
    for (const auto& l : lines_) total_chars += utf8::distance(l.begin(), l.end());
    for (const auto& l : paste_lines) total_chars += utf8::distance(l.begin(), l.end());
    
    if (symbols_limit != -1 && total_chars > (size_t)symbols_limit) {
        return; // We do not insert it if the limit is exceeded.
    }

    if (paste_lines.size() == 1) {
        size_t insert_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
        lines_[cursor_row].insert(insert_byte, paste_lines[0]);
        cursor_col += utf8::distance(paste_lines[0].begin(), paste_lines[0].end());
    } else {
        // We insert several lines
        size_t insert_byte = get_byte_pos_for_char_pos(lines_[cursor_row], cursor_col);
        std::string tail = lines_[cursor_row].substr(insert_byte);
        lines_[cursor_row].erase(insert_byte);
        lines_[cursor_row] += paste_lines[0];
        std::vector<std::string> new_lines;
        new_lines.reserve(paste_lines.size() - 1);
        for (size_t i = 1; i < paste_lines.size() - 1; ++i) {
            new_lines.push_back(paste_lines[i]);
        }
        if (paste_lines.size() > 1) {
            new_lines.push_back(paste_lines.back() + tail);
        }
        lines_.insert(lines_.begin() + cursor_row + 1, new_lines.begin(), new_lines.end());
        cursor_row += paste_lines.size() - 1;
        // Cursor at the end of the inserted block
        cursor_col = utf8::distance(paste_lines.back().begin(), paste_lines.back().end());
        invalidate_max_width_cache();
    }

    update_scroll_areas();
    scroll_to_cursor();
    redraw();
    if (change_callback) change_callback();
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
    int vert_area = std::max(0, total_height - content_height);
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
        auto text_width = get_text_width(line, line.size(), font_);
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
    if (auto_scroll_timer_ && auto_scroll_type_ == auto_scroll_type::idle) {
        auto_scroll_type_ = up ? auto_scroll_type::up : auto_scroll_type::down;
        auto_scroll_timer_->start(80); // 80ms interval (12.5 lines per sec)
    }
}

void input::start_auto_hscroll(bool left)
{
    if (auto_scroll_timer_ && auto_scroll_type_ == auto_scroll_type::idle) {
        auto_scroll_type_ = left ? auto_scroll_type::left : auto_scroll_type::right;
        auto_scroll_timer_->start(80); // 80ms interval (12.5 symbols per sec)
    }
}

void input::stop_auto_scroll()
{
    if (auto_scroll_timer_ && auto_scroll_type_ != auto_scroll_type::idle) {
        auto_scroll_type_ = auto_scroll_type::idle;
        auto_scroll_timer_->stop();
    }
}

void input::on_auto_scroll()
{
    if (auto_scroll_type_ != auto_scroll_type::idle && !selecting) {
        auto_scroll_type_ = auto_scroll_type::idle;
        return;
    }

    switch (auto_scroll_type_)
    {
        case auto_scroll_type::up:
            if (cursor_row > 0) {
                --cursor_row;
                cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                select_end_row = cursor_row;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
            }
            else {
                // If it is already on the first line, move the cursor to the beginning of the line
                cursor_col = 0;
                select_end_row = cursor_row;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
                auto_scroll_type_ = auto_scroll_type::idle;
            }
        break;
        case auto_scroll_type::down:
            if (cursor_row + 1 < lines_.size()) {
                ++cursor_row;
                cursor_col = std::min(cursor_col, static_cast<size_t>(utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end())));
                select_end_row = cursor_row;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
            }
            else {
                // If it is already on the last line, move the cursor to the end of the line
                cursor_col = utf8::distance(lines_[cursor_row].begin(), lines_[cursor_row].end());
                select_end_row = cursor_row;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
                auto_scroll_type_ = auto_scroll_type::idle;
            }
        break;
        case auto_scroll_type::left:
            if (cursor_col > 0) {
                --cursor_col;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
            }
            else {
                // If you are already in the first position, move the cursor up
                if (cursor_row > 0)
                {
                    --cursor_row;
                    cursor_col = 0;
                    select_end_row = cursor_row;
                    select_end_col = cursor_col;
                    scroll_to_cursor();
                    redraw();
                }
                auto_scroll_type_ = auto_scroll_type::idle;
            }
        break;
        case auto_scroll_type::right:
            if (cursor_col < lines_[cursor_row].size()) {
                ++cursor_col;
                select_end_col = cursor_col;
                scroll_to_cursor();
                redraw();
            }
            else {
                // If you are already in the last position, move the cursor down
                if (cursor_row < lines_.size())
                {
                    ++cursor_row;
                    cursor_col = 0;
                    select_end_row = cursor_row;
                    select_end_col = cursor_col;
                    scroll_to_cursor();
                    redraw();
                }
                auto_scroll_type_ = auto_scroll_type::idle;
            }
        break;
    }
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

    const auto& line = lines_[cursor_row];
    
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
