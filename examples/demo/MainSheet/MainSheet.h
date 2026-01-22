//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#pragma once

#include <wui/window/window.hpp>

#include <wui/control/image.hpp>
#include <wui/control/text.hpp>
#include <wui/control/button.hpp>

#include <wui/control/scroll.hpp>

#include <wui/common/about.hpp>
#include <wui/system/uri_tools.hpp>
#include <wui/locale/locale.hpp>

#include <Resource.h>

class MainSheet
{
public:
    MainSheet();

    void Run(std::weak_ptr<wui::window> parentWindow_);
    void End();

    void UpdateSize(int32_t width, int32_t height);

private:
    std::weak_ptr<wui::window> parentWindow_;
    
    std::shared_ptr<wui::image> logoImage = std::make_shared<wui::image>(IMG_LOGO);
    std::shared_ptr<wui::text> welcomeText = std::make_shared<wui::text>(wui::locale("main_sheet", "main_title"), wui::hori_alignment::left, wui::vert_alignment::top, "h1_text");
    
    std::shared_ptr<wui::text> wuiInfoText = std::make_shared<wui::text>(wui::about::full_name + std::string("\n") + wui::about::version, wui::hori_alignment::left, wui::vert_alignment::top);

    std::shared_ptr<wui::button> mainSiteAnchor = std::make_shared<wui::button>(wui::about::web, []() { wui::open_uri(wui::about::web); }, wui::button_view::anchor);

    std::shared_ptr<wui::scroll> vertScroll = std::make_shared<wui::scroll>(1000, 0, wui::orientation::vertical, [this](wui::scroll_state, int32_t v) { welcomeText->set_text(std::to_string(v)); });
    std::shared_ptr<wui::scroll> horScroll = std::make_shared<wui::scroll>(1000, 1000, wui::orientation::horizontal, [this](wui::scroll_state, int32_t v) { welcomeText->set_text(std::to_string(v)); });
};
