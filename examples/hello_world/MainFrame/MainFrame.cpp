//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/config/config.hpp>

#include <wui/theme/theme.hpp>
#include <wui/theme/theme_selector.hpp>

#include <wui/locale/locale.hpp>
#include <wui/locale/locale_selector.hpp>

#include <wui/common/flag_helpers.hpp>

#include <wui/framework/framework.hpp>

#include <wui/system/tools.hpp>

#include <MainFrame/MainFrame.h>

#include <Resource.h>

#include <iostream>

MainFrame::MainFrame()
{
    window->subscribe(std::bind(&MainFrame::ReceiveEvents,
        this,
        std::placeholders::_1),
        wui::flags_map<wui::event_type>(3, wui::event_type::internal, wui::event_type::system, wui::event_type::keyboard));

    window->add_control(logoImage,         { 0 });
    window->add_control(whatsYourNameText, { 0 });
    window->add_control(userNameInput,     { 0 });
    window->add_control(okButton,          { 0 });

    window->set_default_push_control(okButton);

    window->set_min_size(WND_WIDTH - 1, WND_HEIGHT - 1);
}

void MainFrame::Run()
{
    UpdateControlsPosition();

    window->set_control_callback([&](wui::window_control control, std::string &tooltip_text, bool &continue_) {
        switch (control)
        {
            case wui::window_control::theme:
            {
                wui::error err;

                auto nextTheme = wui::get_next_app_theme();
                wui::set_default_theme_from_name(nextTheme, err);
                if (!err.is_ok())
                {
                    std::cerr << err.str() << std::endl;
                    return;
                }

                wui::config::set_string("User", "Theme", nextTheme);

                window->update_theme();
            }
            break;
			case wui::window_control::lang:
			{
                wui::error err;

                auto nextLocale = wui::get_next_app_locale();
                wui::set_locale_from_type(nextLocale, err);
                if (!err.is_ok())
                {
                    std::cerr << err.str() << std::endl;
                    return;
                }

                wui::config::set_int("User", "Locale", static_cast<int32_t>(nextLocale));

				tooltip_text = wui::locale("window", "switch_lang");

				window->set_caption(wui::locale("main_frame", "caption"));
				whatsYourNameText->set_text(wui::locale("main_frame", "whats_your_name_text"));
				okButton->set_caption(wui::locale("main_frame", "ok_button"));
			}
			break;
            case wui::window_control::close:
                if (!user_approve_close)
                {
                    continue_ = false;
                    messageBox->show(wui::locale("main_frame", "confirm_close_text"),
                        wui::locale("main_frame", "cross_message_caption"),
                        wui::message_icon::information, wui::message_button::yes_no,
                        [this, &continue_](wui::message_result r) {
							if (r == wui::message_result::yes)
							{
                                user_approve_close = true;
                                window->destroy();
							}
                        });
                }
            break;
        }
    });

    auto width = wui::config::get_int("MainFrame", "Width", WND_WIDTH);
    auto height = wui::config::get_int("MainFrame", "Height", WND_HEIGHT);

    window->init(wui::locale("main_frame", "caption"), { -1, -1, width, height },
        wui::flags_map<wui::window_style>(4, wui::window_style::frame, wui::window_style::switch_theme_button, wui::window_style::switch_lang_button, wui::window_style::border_all),
        [this]() { wui::framework::stop(); });
}

void MainFrame::ReceiveEvents(const wui::event &ev)
{
    if (ev.type == wui::event_type::internal)
    {
        switch (ev.internal_event_.type)
        {
            case wui::internal_event_type::window_created:
        
            break;
            case wui::internal_event_type::size_changed:        
                if (window->state() == wui::window_state::normal &&
                    ev.internal_event_.x > 0 && ev.internal_event_.y > 0)
                {
                    wui::config::set_int("MainFrame", "Width", ev.internal_event_.x);
                    wui::config::set_int("MainFrame", "Height", ev.internal_event_.y);
                }
                UpdateControlsPosition();
            break;
            case wui::internal_event_type::window_expanded:
            case wui::internal_event_type::window_normalized:
                UpdateControlsPosition();
            break;
            case wui::internal_event_type::window_minimized:
            break;
        }
    }
}

void MainFrame::UpdateControlsPosition()
{
    const auto width = window->position().width(), height = window->position().height();

    const int32_t top = 40, element_height = 40, space = 30;

    wui::rect pos = { space, top, width - space, top + element_height };
    whatsYourNameText->set_position(pos);
    wui::line_up_top_bottom(pos, element_height, space);
    userNameInput->set_position(pos);
    wui::line_up_top_bottom(pos, element_height * 2, space);
    
    int32_t center = width / 2;

    pos.left = center - element_height, pos.right = center + element_height;

    logoImage->set_position(pos);

    okButton->set_position({center - 90,
        height - element_height - space,
        center + 90,
        height - space
    });
}

void MainFrame::OnOK()
{
    wui::config::set_string("User", "Name", userNameInput->text());

    messageBox->show(wui::locale("main_frame", "hello_text") + userNameInput->text(),
        wui::locale("main_frame", "ok_message_caption"), wui::message_icon::information, 
        wui::message_button::ok, [this](wui::message_result) {
            user_approve_close = true;
            window->destroy();
        });
}