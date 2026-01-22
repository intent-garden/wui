//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <MainSheet/MainSheet.h>

#include <wui/system/tools.hpp>

#include <iostream>

MainSheet::MainSheet()
    : 
    mainSiteAnchor(std::make_shared<wui::button>(wui::about::web, [](){ wui::open_uri(wui::about::web); }, wui::button_view::anchor))
{
    auto e = logoImage->get_error();
    if (!e.is_ok())
    {
        std::cerr << e.str() << std::endl;
    }
}

void MainSheet::Run(std::weak_ptr<wui::window> parentWindow__)
{
    parentWindow_ = parentWindow__;

    auto parentWindow = parentWindow_.lock();
    if (parentWindow)
    {
        auto parentPos = parentWindow->position();
        auto width = parentPos.width(), height = parentPos.height();

        parentWindow->add_control(logoImage, { 0 });
        parentWindow->add_control(welcomeText, { 0 });
        parentWindow->add_control(wuiInfoText, { 0 });
        parentWindow->add_control(mainSiteAnchor, { 0 });

        parentWindow->add_control(vertScroll, { 0 });
        parentWindow->add_control(horScroll,  { 0 });

        UpdateSize(width, height);
    }

    logoImage->update_theme();
    welcomeText->update_theme();
    mainSiteAnchor->update_theme();
    wuiInfoText->update_theme();

    vertScroll->update_theme();
    horScroll->update_theme();
}

void MainSheet::End()
{
    auto parentWindow = parentWindow_.lock();
    if (parentWindow)
    {
        parentWindow->remove_control(logoImage);
        parentWindow->remove_control(welcomeText);
        parentWindow->remove_control(wuiInfoText);
        parentWindow->remove_control(mainSiteAnchor);

        parentWindow->remove_control(vertScroll);
        parentWindow->remove_control(horScroll);
    }
}

void MainSheet::UpdateSize(int32_t width, int32_t height)
{
    logoImage->set_position({ 10, 100, 60, 150 });
    welcomeText->set_position({ 70, 122, width - 10, 150 });
    mainSiteAnchor->set_position({ 10, height - 40, width - 10, height - 20 });
    wuiInfoText->set_position({ 10, 180, width - 10, 180 + 50 });

    vertScroll->set_position({ width - 50, 100, width - 35, height - 100 });
    horScroll->set_position({ 10, height - 80, width - 10, height - 66 });
}
