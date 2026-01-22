//
// Copyright (c) 2023 Anton Golovkov (udattsk at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//

#include <ButtonSheet/ButtonSheet.h>

#include <Resource.h>

#include <wui/common/about.hpp>
#include <wui/locale/locale.hpp>
#include <wui/system/tools.hpp>
#include <wui/system/uri_tools.hpp>

#include <iostream>

ButtonSheet::ButtonSheet()
    : parentWindow_(),
    buttonsText(std::make_shared<wui::text>("", wui::hori_alignment::left, wui::vert_alignment::top, "h1_text")),
    simpleButton0(std::make_shared<wui::button>(wui::locale("button_sheet", "simple0"), [this]() { buttonsText->set_text("First button pushed"); }, wui::button_view::text)),
    simpleButton1(std::make_shared<wui::button>(wui::locale("button_sheet", "simple1"), [this]() { buttonsText->set_text("Second button pushed"); }, wui::button_view::text)),
    imageButton(std::make_shared<wui::button>("", [this]() { buttonsText->set_text("Image button pushed"); }, wui::button_view::image, IMG_LOGO, 32)),
    imageRightTextButton(std::make_shared<wui::button>(wui::locale("button_sheet", "right_text"), [this]() { buttonsText->set_text("Image with right text button pushed"); }, wui::button_view::image_right_text, IMG_LOGO, 32)),
    imageBottomTextButton(std::make_shared<wui::button>(wui::locale("button_sheet", "bottom_text"), [this]() { buttonsText->set_text("Image with bottom text button pushed"); }, wui::button_view::image_bottom_text, IMG_LOGO, 32, "rounded__green_button")),
    switcherButton(std::make_shared<wui::button>(wui::locale("button_sheet", "switcher_text"), [this]() { buttonsText->set_text("Switcher button pushed"); }, wui::button_view::switcher)),
    radioButton0(std::make_shared<wui::button>(wui::locale("button_sheet", "radio0_text"), [this]() { buttonsText->set_text("Radio first pushed"); radioButton1->turn(!radioButton0->turned()); }, wui::button_view::radio)),
    radioButton1(std::make_shared<wui::button>(wui::locale("button_sheet", "radio1_text"), [this]() { buttonsText->set_text("Radio second pushed"); radioButton0->turn(!radioButton1->turned()); }, wui::button_view::radio)),
    anchorButton(std::make_shared<wui::button>(wui::locale("button_sheet", "anchor_text"), [this]() { buttonsText->set_text("Anchor pushed"); }, wui::button_view::anchor)),
    sheetButton0(std::make_shared<wui::button>(wui::locale("button_sheet", "sheet0_text"), [this]() { buttonsText->set_text("First sheet pushed"); sheetButton0->turn(true); sheetButton1->turn(false); }, wui::button_view::sheet)),
    sheetButton1(std::make_shared<wui::button>(wui::locale("button_sheet", "sheet1_text"), [this]() { buttonsText->set_text("Second sheet pushed"); sheetButton0->turn(false); sheetButton1->turn(true); }, wui::button_view::sheet))
{
    radioButton0->turn(true);
    sheetButton0->turn(true);

    auto e = imageButton->get_error();
    if (!e.is_ok())
    {
        std::cerr << e.str() << std::endl;
    }
}

void ButtonSheet::Run(std::weak_ptr<wui::window> parentWindow__)
{
    parentWindow_ = parentWindow__;

    auto parentWindow = parentWindow_.lock();
    if (parentWindow)
    {
        auto parentPos = parentWindow->position();
        auto width = parentPos.width(), height = parentPos.height();

        parentWindow->add_control(buttonsText, { 10, 80, 800, 140 });

        parentWindow->add_control(simpleButton0, { 10, 150, 200, 180 });
        parentWindow->add_control(simpleButton1, { 220, 150, 420, 180 });

        parentWindow->add_control(imageButton, { 10, 200, 10 + 48, 200 + 48 });
        parentWindow->add_control(imageRightTextButton, { 70, 200, 300, 200 + 48 });

        parentWindow->add_control(imageBottomTextButton, { 10, 260, 200, 260 + 80 });

        parentWindow->add_control(switcherButton, { 10, 360, 200, 360 + 30 });

        parentWindow->add_control(radioButton0, { 10, 400, 150, 400 + 30 });
        parentWindow->add_control(radioButton1, { 160, 400, 310, 400 + 30 });

        parentWindow->add_control(anchorButton, { 10, 450, 100, 450 + 30 });

        parentWindow->add_control(sheetButton0, { 10, 500, 100, 500 + 30 });
        parentWindow->add_control(sheetButton1, { 110, 500, 210, 500 + 30 });
    }
    
    sheetButton0->update_theme();
    sheetButton1->update_theme();
    buttonsText->update_theme();
    simpleButton0->update_theme();
    simpleButton1->update_theme();
    imageButton->update_theme();
    imageRightTextButton->update_theme();
    imageBottomTextButton->update_theme();
    switcherButton->update_theme();
    radioButton0->update_theme();
    radioButton1->update_theme();
    anchorButton->update_theme();
}

void ButtonSheet::End()
{
    auto parentWindow = parentWindow_.lock();
    if (parentWindow)
    {
        parentWindow->remove_control(sheetButton1);
        parentWindow->remove_control(sheetButton0);
        parentWindow->remove_control(anchorButton);
        parentWindow->remove_control(radioButton1);
        parentWindow->remove_control(radioButton0);
        parentWindow->remove_control(switcherButton);
        parentWindow->remove_control(imageBottomTextButton);
        parentWindow->remove_control(imageRightTextButton);
        parentWindow->remove_control(imageButton);
        parentWindow->remove_control(simpleButton1);
        parentWindow->remove_control(simpleButton0);
        parentWindow->remove_control(buttonsText);
    }
}

void ButtonSheet::UpdateSize(int32_t width, int32_t height)
{
}