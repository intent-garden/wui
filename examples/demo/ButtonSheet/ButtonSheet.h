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

#include <wui/control/text.hpp>
#include <wui/control/button.hpp>


class ButtonSheet
{
public:
    ButtonSheet();

    void Run(std::weak_ptr<wui::window> parentWindow_);
    void End();

    void UpdateSize(int32_t width, int32_t height);

private:
    std::weak_ptr<wui::window> parentWindow_;
    
    std::shared_ptr<wui::text> buttonsText;
    std::shared_ptr<wui::button> simpleButton0, simpleButton1;
    std::shared_ptr<wui::button> imageButton;
    std::shared_ptr<wui::button> imageRightTextButton;
    std::shared_ptr<wui::button> imageBottomTextButton;
    std::shared_ptr<wui::button> switcherButton;
    std::shared_ptr<wui::button> radioButton0, radioButton1;
    std::shared_ptr<wui::button> anchorButton;
    std::shared_ptr<wui::button> sheetButton0, sheetButton1;
};
