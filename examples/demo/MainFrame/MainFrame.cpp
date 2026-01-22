//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <wui/framework/framework.hpp>

#include <wui/config/config.hpp>

#include <wui/theme/theme.hpp>
#include <wui/theme/theme_selector.hpp>

#include <wui/locale/locale.hpp>
#include <wui/locale/locale_selector.hpp>

#include <wui/common/flag_helpers.hpp>

#include <MainFrame/MainFrame.h>

#include <Resource.h>

#include <iostream>

MainFrame::MainFrame()
    : window(std::make_shared<wui::window>()),
    
    mainSheet    (std::make_shared<wui::button>(wui::locale("main_frame", "main_sheet_"),   [this](){ sheet = Sheet::Main;   UpdateSheets(); }, wui::button_view::sheet)),
    windowSheet  (std::make_shared<wui::button>(wui::locale("main_frame", "window_sheet_"), [this](){ sheet = Sheet::Window; UpdateSheets(); }, wui::button_view::sheet)),
    buttonSheet  (std::make_shared<wui::button>(wui::locale("main_frame", "button_sheet_"), [this](){ sheet = Sheet::Button; UpdateSheets(); }, wui::button_view::sheet)),
    inputSheet   (std::make_shared<wui::button>(wui::locale("main_frame", "input_sheet_"),  [this](){ sheet = Sheet::Input;  UpdateSheets(); }, wui::button_view::sheet)),
    listSheet    (std::make_shared<wui::button>(wui::locale("main_frame", "list_sheet_"),   [this](){ sheet = Sheet::List;   UpdateSheets(); }, wui::button_view::sheet)),
    menuSheet    (std::make_shared<wui::button>(wui::locale("main_frame", "menu_sheet_"),   [this](){ sheet = Sheet::Menu;   UpdateSheets(); }, wui::button_view::sheet)),
    panelSheet   (std::make_shared<wui::button>(wui::locale("main_frame", "other_sheet_"),  [this](){ sheet = Sheet::Others; UpdateSheets(); }, wui::button_view::sheet)),
    
    accountButton(std::make_shared<wui::button>(wui::locale("main_frame", "account_btn"),   []() {}, wui::button_view::image, IMG_ACCOUNT, 32, wui::button::tc_tool)),
    menuButton   (std::make_shared<wui::button>(wui::locale("main_frame", "main_menu"),     []() {}, wui::button_view::image, IMG_MENU,    32, wui::button::tc_tool)),
    
    sheet(Sheet::Main),

    mainSheetImpl(),
    buttonSheetImpl()
{
    window->subscribe(std::bind(&MainFrame::ReceiveEvents,
        this,
        std::placeholders::_1),
        wui::flags_map<wui::event_type>(3, wui::event_type::internal, wui::event_type::system, wui::event_type::keyboard));

    window->add_control(mainSheet,     { 0 });
    window->add_control(windowSheet,   { 0 });
    window->add_control(buttonSheet,   { 0 });
    window->add_control(inputSheet,    { 0 });
    window->add_control(listSheet,     { 0 });
    window->add_control(menuSheet,     { 0 });
    window->add_control(panelSheet,    { 0 });
    window->add_control(accountButton, { 0 });
    window->add_control(menuButton,    { 0 });
}

void MainFrame::Run()
{
    UpdateSheets();

    window->set_control_callback([&](wui::window_control control, std::string &tooltip_text, bool &continue_) {
        switch (control)
        {
            case wui::window_control::theme:
            {
                auto nextTheme = wui::get_next_app_theme();
                wui::error err;
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
            }
            break;
        }

        window->update_theme();
    });

    window->init(wui::locale("main_frame", "caption"), { -1, -1, WND_WIDTH, WND_HEIGHT },
        wui::flags_map<wui::window_style>(3, wui::window_style::frame, wui::window_style::switch_theme_button, wui::window_style::border_all),
        [this]() {
            wui::framework::stop();
    });

    UpdateSheets();
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
                    UpdateSheetsSize();
                }
            break;
            case wui::internal_event_type::window_expanded:
                UpdateSheetsSize();
            break;
            case wui::internal_event_type::window_normalized:
                UpdateSheetsSize();
            break;
            case wui::internal_event_type::window_minimized:
            break;
        }
    }
}

void MainFrame::UpdateSheetsSize()
{
    const auto width = window->position().width(), height = window->position().height();
    const auto sheet_width = (width - 140) / 7;

    const int32_t sheets_top = 35, sheets_height = 30;

    mainSheet->set_position({ 10,                       sheets_top, sheet_width,              sheets_top + sheets_height });
    windowSheet->set_position({ 10 * 2 + sheet_width,     sheets_top, 10 * 2 + sheet_width * 2, sheets_top + sheets_height });
    buttonSheet->set_position({ 10 * 3 + sheet_width * 2, sheets_top, 10 * 3 + sheet_width * 3, sheets_top + sheets_height });
    inputSheet->set_position({ 10 * 4 + sheet_width * 3, sheets_top, 10 * 4 + sheet_width * 4, sheets_top + sheets_height });
    listSheet->set_position({ 10 * 5 + sheet_width * 4, sheets_top, 10 * 5 + sheet_width * 5, sheets_top + sheets_height });
    menuSheet->set_position({ 10 * 6 + sheet_width * 5, sheets_top, 10 * 6 + sheet_width * 6, sheets_top + sheets_height });
    panelSheet->set_position({ 10 * 7 + sheet_width * 6, sheets_top, 10 * 7 + sheet_width * 7, sheets_top + sheets_height });

    const int32_t button_size = 36;

    accountButton->set_position({ width - button_size * 2 - 4, 30, width - button_size - 4, 30 + button_size });
    menuButton->set_position({ width - button_size - 2, 30, width - 2, 30 + button_size });

    mainSheetImpl.UpdateSize(width, height);
    buttonSheetImpl.UpdateSize(width, height);
    inputSheetImpl.UpdateSize(width, height);
}

void MainFrame::UpdateSheets()
{
    mainSheet  ->turn(false);
    windowSheet->turn(false);
    buttonSheet->turn(false);
    inputSheet ->turn(false);
    listSheet  ->turn(false);
    menuSheet  ->turn(false);
    panelSheet ->turn(false);

    switch (sheet)
    {
        case Sheet::Main:
            mainSheet->turn(true);

            buttonSheetImpl.End();
            inputSheetImpl.End();

            mainSheetImpl.Run(window);
        break;
        case Sheet::Window:
            windowSheet->turn(true);
            
            mainSheetImpl.End();
            buttonSheetImpl.End();
            inputSheetImpl.End();
        break;
        case Sheet::Button:
            buttonSheet->turn(true);
            
            mainSheetImpl.End();
            inputSheetImpl.End();

            buttonSheetImpl.Run(window);
        break;
        case Sheet::Input:
            inputSheet->turn(true);

            mainSheetImpl.End();
            buttonSheetImpl.End();

            inputSheetImpl.Run(window);
        break;
        case Sheet::List:
            listSheet->turn(true);

            mainSheetImpl.End();
            buttonSheetImpl.End();
            inputSheetImpl.End();
        break;
        case Sheet::Menu:
            menuSheet->turn(true);

            mainSheetImpl.End();
            buttonSheetImpl.End();
            inputSheetImpl.End();
        break;
        case Sheet::Others:
            panelSheet->turn(true);

            mainSheetImpl.End();
            buttonSheetImpl.End();
            inputSheetImpl.End();
        break;
    }
}

